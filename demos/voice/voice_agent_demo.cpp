/**
 * @example voice_agent_demo.cpp
 * @brief Voice Agent Demo
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/agents/voice_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/http_client.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <chrono>
#include <csignal>
#include <iostream>

using namespace agents;

volatile std::sig_atomic_t g_stop = 0;

void handleStop(int sig) {
    Logger::info("Signal {} raised; Press any key to exit", sig);
    g_stop = sig;
}

int main() {
    // Register signal handler
    std::signal(SIGINT, handleStop);
    std::signal(SIGTERM, handleStop);

    // Initialize the logger
    Logger::init(Logger::Level::INFO);

    // Load user config
    auto& config = ConfigLoader::getInstance();

    // Create LLM based on user choice
    std::shared_ptr<LLMInterface> llm =
        createLLM(config.get("PROVIDER"), config.get("API_KEY"), config.get("MODEL"));

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Set system prompt for the context
    context->setSystemPrompt(config.get("SYSTEM_PROMPT"));

    // Register tools from tool registry
    auto registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // Create the agent
    VoiceAgent::Config cfg;
    cfg.agent_endpoint = "http://localhost:8080/agent";
    cfg.stt_endpoint = "http://localhost:8888/";
    cfg.tts_endpoint = "http://localhost:9999/";
    cfg.api_key = config.get("EDGEAI_API_KEY");

    VoiceAgent agent(context, cfg);

    // Initialize the agent
    agent.init();

    // Run the UI in a separate thread
    httplib::Server svr;
    // NOTE: Access to agent from UI and CLI is mutually exclusive in this demo
    std::thread ui_thread(runUI, "sample_media/ui/index.html", std::ref(svr), std::ref(agent));

    // Get user input
    Logger::info("==================================================");
    Logger::info("                    VOICE AGENT                   ");
    Logger::info("==================================================");
    Logger::info("Press 'a' and speak a question (or 'exit' to quit):");

    std::string user_input;
    while (!g_stop) {
        Logger::info("\n> ");
        std::getline(std::cin, user_input);

        if (user_input.empty()) {
            continue;
        }

        if (user_input == "exit" || user_input == "quit" || user_input == "q") {
            break;
        }

        if (user_input == "a") {
            agent.listen();
            continue;
        }

        // otherwise run agent with text provided
        agent.runAsync(user_input, [&](const JsonObject& jsonPartial) {
            if (!jsonPartial.empty() && jsonPartial.contains("part") && !jsonPartial["part"].is_null()) {
                auto part = jsonPartial["part"].get<std::string>();
                std::cout << part << std::flush;
            }
            // Note: Final result is also sent via this callback with "answer"
            if (!jsonPartial.empty() && jsonPartial.contains("answer")) {
                std::cout << std::endl;
            }
        });
    }

    // Cleanup
    agent.stop();
    svr.stop();
    ui_thread.join();

    return EXIT_SUCCESS;
}