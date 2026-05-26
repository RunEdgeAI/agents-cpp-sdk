/**
 * @example routing_example.cpp
 * @brief Routing Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>
#include <agents-cpp/workflows/routing_workflow.h>

#include <iostream>
#include <string>

using namespace agents;

int main(int argc, char* argv[]) {
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
        return EXIT_FAILURE;
    }

    // Create LLM
    auto llm = createLLM("google", api_key, "gemini-2.5-flash");

    // Configure LLM options
    LLMOptions options;
    options.temperature = 0.2;
    options.max_tokens = 2048;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Register some tools
    context->registerTool(tools::createWebSearchTool(llm));
    context->registerTool(tools::createWikipediaTool());

    // Create routing workflow
    workflows::RoutingWorkflow router(context);

    // Set router prompt
    router.setRouterPrompt(
        "You are a routing assistant that examines user queries and classifies them into appropriate categories. "
        "Determine the most suitable category for handling the user's query based on the available routes."
    );

    // Add routes for different query types
    router.addRoute(
        "factual_query",
        "Questions about facts, events, statistics, or general knowledge",
        [context](const std::string& input, const JsonObject& routing_info) -> JsonObject {
            Logger::debug("Routing info: {}", routing_info.dump(2));
            Logger::info("Handling factual query: {}", input);

            auto wiki_tool = tools::createWikipediaTool();
            ToolResult result = wiki_tool->execute({{"query", input}});

            JsonObject response;
            response["answer"] = "Based on research: " + result.content;
            return response;
        }
    );

    router.addRoute(
        "opinion_query",
        "Questions seeking opinions, evaluations, or judgments on topics",
        [context](const std::string& input, const JsonObject& routing_info) -> JsonObject {
            Logger::debug("Routing info: {}", routing_info.dump(2));
            Logger::info("Handling opinion query: {}", input);

            // Create specific context for opinion handling
            auto opinion_context = std::make_shared<Context>(*context);
            opinion_context->setSystemPrompt(
                "You are a balanced and thoughtful assistant that provides nuanced perspectives on complex topics. "
                "Consider multiple viewpoints and provide balanced opinions."
            );

            // Get response from LLM
            LLMResponse llm_response = opinion_context->getLLM()->chat(input);
            std::string response = llm_response.content;

            JsonObject result;
            result["answer"] = "Opinion analysis: " + response;
            return result;
        }
    );

    router.addRoute(
        "technical_query",
        "Questions about technical topics, programming, or specialized domains",
        [context](const std::string& input, const JsonObject& routing_info) -> JsonObject {
            Logger::debug("Routing info: {}", routing_info.dump(2));
            Logger::info("Handling technical query: {}", input);

            // Create specific context for technical handling
            auto technical_context = std::make_shared<Context>(*context);
            technical_context->setSystemPrompt(
                "You are a technical expert assistant that provides accurate and detailed information on technical topics. "
                "Focus on clarity, precision, and correctness."
            );

            // Get response from LLM
            LLMResponse llm_response = technical_context->getLLM()->chat(input);
            std::string response = llm_response.content;

            JsonObject result;
            result["answer"] = "Technical explanation: " + response;
            return result;
        }
    );

    // Set default route
    router.setDefaultRoute([context](const std::string& input, const JsonObject& routing_info) -> JsonObject {
        Logger::debug("Routing info: {}", routing_info.dump(2));
        Logger::info("Handling with default route: {}", input);

        // Get response from LLM
        LLMResponse llm_response = context->getLLM()->chat(input);
        std::string response = llm_response.content;

        JsonObject result;
        result["answer"] = "General response: " + response;
        return result;
    });

    // Process user inputs until exit
    Logger::info("Enter queries (or 'exit' to quit):");
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
            // Run the routing workflow
            JsonObject result = router.run(user_input);

            // Display the result
            Logger::info("\nResponse: {}", result["answer"].get<std::string>());
            Logger::info("--------------------------------------");
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    return EXIT_SUCCESS;
}
