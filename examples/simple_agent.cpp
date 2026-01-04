/**
 * @example simple_agent.cpp
 * @brief Simple Chatbot Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/agents/actor_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <iostream>

using namespace agents;

// Main agent task
Task<int> runAgentApp(int argc, char* argv[]) {
    // Initialize the logger
    Logger::init(Logger::Level::INFO);

    // Get API key from .env, environment, or command line
    std::string api_key;
    auto& config = ConfigLoader::getInstance();

    // Try to get API key from config or environment
    api_key = config.get("GEMINI_API_KEY", "");

    // If not found, check command line
    if (api_key.empty() && argc > 1) {
        api_key = argv[1];
    }

    // Still not found, show error and exit
    if (api_key.empty()) {
        Logger::error("API key not found. Please:");
        Logger::error("1. Create a .env file with GEMINI_API_KEY=your_key, or");
        Logger::error("2. Set the GEMINI_API_KEY environment variable, or");
        Logger::error("3. Provide an API key as a command line argument");
        co_return EXIT_FAILURE;
    }

    // Create the context
    auto context = std::make_shared<Context>();

    // Configure the LLM
    auto llm = createLLM("google", api_key, "gemini-2.5-flash");

    // Configure LLM options
    LLMOptions options;
    options.temperature = (0.7);
    llm->setOptions(options);

    context->setLLM(llm);

    // Register some tools
    context->registerTool(tools::createWikipediaTool());
    context->registerTool(tools::createWebSearchTool(llm));
    context->registerTool(tools::createSummarizationTool(llm));

    // Create the agent
    ActorAgent agent(context);

    // Personalize the actor agent's character
    agent.setAgentPrompt(
        "You are a friendly assistant that helps users find information and answer questions. "
        "Use the tools available to you to gather information and provide comprehensive answers. "
        "Be clear and concise in your responses."
    );

    // Set up callbacks
    agent.setStatusCallback([](const std::string& status) {
        Logger::info("Agent status: {}", status);
    });

    // Initialize the agent
    agent.init();

    // Get user input
    Logger::info("Enter a question or task for the agent (or 'exit' to quit):");
    std::string user_input;
    while (true) {
        Logger::info("> ");
        std::getline(std::cin, user_input);

        if (user_input == "exit" || user_input == "quit" || user_input == "q") {
            break;
        }

        if (user_input.empty()) {
            continue;
        }

        try {
            // Run the agent with coroutines
            JsonObject result = co_await agent.run(user_input);

            // Display the final result
            Logger::info("Final Result:\n{}", result["answer"].get<std::string>());
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    co_return EXIT_SUCCESS;
}

// Entry point
int main(int argc, char* argv[]) {
    // Use blockingWait to execute the coroutine
    return blockingWait(runAgentApp(argc, argv));
}
