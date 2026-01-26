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

    // Create LLM based on user choice
    std::shared_ptr<LLMInterface> llm;

    // Check if we have any API keys configured
    auto& config = ConfigLoader::getInstance();

    // Use API keys available
    try {
        llm = createLLM(config.get("PROVIDER"), config.get("API_KEY"), config.get("MODEL"));
    } catch (const std::exception& e) {
        Logger::error("Error creating LLM: {}", e.what());
        Logger::error("Please ensure the appropriate API key is set in the environment.");
        return EXIT_FAILURE;
    }

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Set system prompt for the context
    context->setSystemPrompt(
        "You are a friendly and advanced voice assistant built by Edge AI. "
        "Be brief, concise, and to the point in your responses. "
        "You must ALWAYS respond in at most 30 words. "
        "You are encouraged to respond with follow-on questions if you are unsure about something. "
        "Your output will go into a TTS engine so do not include any"
        "special tokens or markup. Do not bold, italicize, or add any other formatting to your response. "
    );

    // Register tools from tool registry
    auto registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // Create the agent
    VoiceAgent::Config cfg;
    cfg.agent_endpoint = "http://127.0.0.1:8080/agent";
    cfg.stt_endpoint = "http://127.0.0.1:8888/";
    cfg.tts_endpoint = "http://127.0.0.1:9999/";

    VoiceAgent agent(context, cfg);

    // OPTIONAL: Set the agent prompt (this extends the context system prompt for the agent)
    // agent.setAgentPrompt();

    // Initialize the agent
    agent.init();

    // Run the UI in a separate thread
    httplib::Server svr;
    // NOTE: Access to agent from UI and CLI is mutually exclusive in this demo
    std::thread ui_thread(runUI, "./index.html", std::ref(svr), std::ref(agent));

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
        JsonObject json = blockingWait(agent.run(user_input));
        Logger::info("Assistant: {}", json["answer"].get<std::string>());
    }

    agent.stop();
    // Stop the UI server
    svr.stop();
    ui_thread.join();

    return EXIT_SUCCESS;
}