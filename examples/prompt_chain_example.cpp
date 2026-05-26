/**
 * @example prompt_chain_example.cpp
 * @brief Prompt Chain Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/workflows/prompt_chaining_workflow.h>

#include <iostream>

using namespace agents;
using namespace agents::workflows;

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
    auto llm = createLLM("google", api_key, "gemini-2.0-flash");

    // Configure LLM options
    LLMOptions options;
    options.temperature = 0.3;
    options.max_tokens = 5120;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Create the prompt chain workflow
    PromptChainingWorkflow chain(context);

    // Add a step to generate a document outline
    chain.addStep(
        "outline",
        "You are an expert document planner. Your task is to create a detailed outline for a document about '{input}'. \n\n"
        "The outline should include main sections and subsections. Be comprehensive but focused."
    );

    // Add a step with a gate function to validate the outline
    chain.addStep(
        "validate_outline",
        "You are a document validator. Your task is to evaluate an outline and determine if it's comprehensive and well-structured. "
        "Check if it covers all important aspects of the topic and has a logical flow. Outline: {context}",
        // Validator: check if the outline is approved
        [](const JsonObject& result) -> bool {
            std::string response = result["response"].get<std::string>();
            // For testing, accept any response that contains "outline" or "step"
            return response.find("approved") != std::string::npos ||
                   response.find("looks good") != std::string::npos ||
                   response.find("comprehensive") != std::string::npos;
        }
    );

    // Add a step to write the document based on the outline
    chain.addStep(
        "write_document",
        "You are an expert content writer. Your task is to write a comprehensive document following the provided outline: {context}. \n\n"
        "Make sure to cover each section in detail and maintain a professional tone."
    );

    // Add a step to proofread and improve the document
    chain.addStep(
        "proofread",
        "You are a professional content editor. Your task is to proofread and improve the provided document: {context}. \n\n"
        "Fix any grammatical errors, improve clarity and flow, and ensure consistency. "
        "Assert a proper conclusion exists and no cut off content at the end of the document. "
    );

    // Set a callback for intermediate steps
    chain.setStepCallback([](const std::string& step_name, const JsonObject& result) {
        Logger::debug("Step result: {}", result.dump(2));
        Logger::info("Completed step: {}", step_name);
        Logger::info("--------------------------------------");
    });

    // Get user input
    Logger::info("Enter a topic for document generation:");
    std::string user_input;
    std::getline(std::cin, user_input);

    if (user_input == "exit" || user_input == "quit" || user_input == "q") {
        return EXIT_SUCCESS;
    }

    try {
        // Run the chain
        JsonObject result = chain.run(user_input);

        // Display the final document
        if (result.contains("proofread") && result["proofread"].contains("response")) {
            Logger::info("\nFinal Document:\n{}", result["proofread"]["response"].get<std::string>());
        } else if (result.contains("response")) {
            Logger::info("\nFinal Document:\n{}", result["response"].get<std::string>());
        } else {
            Logger::info("\nFinal Result:\n{}", result.dump());
        }
    } catch (const std::exception& e) {
        Logger::error("Error parsing result: {}", e.what());
    }

    return EXIT_SUCCESS;
}