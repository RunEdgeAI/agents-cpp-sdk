/**
 * @file openclaw_demo.cpp
 * @brief OpenClaw-style agent — composable workspace prompts, skills, memory, hooks
 *
 * Configure via .env (copy from .env.template):
 *   PROVIDER=anthropic
 *   API_KEY=sk-ant-...
 *   MODEL=claude-haiku-4-5-20251001
 *
 * Run:
 *   bazel run demos/openclaw:openclaw_demo
 *
 * Behaviour, memory, and skills are plain Markdown files in .agents/ —
 * edit them without recompiling.
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 */
#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <iostream>

using namespace agents;

int main() {
    Logger::init(Logger::Level::INFO);

    // ── Config ────────────────────────────────────────────────────────────────
    auto& config = ConfigLoader::getInstance();
    std::shared_ptr<LLMInterface> llm =
        createLLM(config.get("PROVIDER"), config.get("API_KEY"), config.get("MODEL"));

    // ── Context ───────────────────────────────────────────────────────────────
    auto context = std::make_shared<Context>();
    context->setLLM(llm);
    context->setTrustLevel(TrustLevel::SANDBOXED);

    // Reads AGENTS.md, SOUL.md, USER.md, MEMORY.md from .agents/
    context->loadWorkspaceConfig();

    // ── Tools ─────────────────────────────────────────────────────────────────
    tools::ToolRegistry registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // ── Skills ────────────────────────────────────────────────────────────────
    // Skills are .md files with optional frontmatter gates:
    //   requires.bins: [git]        — skipped if binary not found in PATH
    //   requires.env:  [DEPLOY_KEY] — skipped if env var is not set
    context->loadSkills({
        ".agents/skills",                  // workspace-level skills
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.agents/skills",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.claude/skills",
    });

    // ── Hooks ─────────────────────────────────────────────────────────────────
    context->setOnBeforeToolExecution([](const std::string& name, const JsonObject&) {
        Logger::info("[tool] {}", name);
    });

    context->setOnBeforeCompaction([&](std::vector<Message>&) {
        context->getMemory()->flushToPersistent(llm);
    });

    // ── Run ───────────────────────────────────────────────────────────────────
    AutonomousAgent agent(context);
    agent.setPlanningStrategy(AutonomousAgent::PlanningStrategy::REACT);
    agent.init();

    Logger::info("OpenClaw agent ready. Type your request (or 'exit' to quit)");

    std::string user_input;
    while (true) {
        Logger::chunk("\n> ");
        std::getline(std::cin, user_input);

        if (user_input.empty()) continue;
        if (user_input == "exit" || user_input == "quit" || user_input == "q") break;

        agent.runAsync(user_input, [](const JsonObject& update) {
            if (update.contains("part") && !update["part"].is_null()) {
                Logger::chunk(update["part"].get<std::string>());
            }
            if (update.contains("answer")) {
                Logger::chunk("\n");
            }
        });
    }

    return EXIT_SUCCESS;
}