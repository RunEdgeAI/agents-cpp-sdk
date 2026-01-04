/**
 * @example orchestrator_example.cpp
 * @brief Orchestrator Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>
#include <agents-cpp/workflows/orchestrator_workflow.h>

#include <iostream>

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
    options.temperature = 0.3;
    options.max_tokens = 2048;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Register tools
    context->registerTool(tools::createWebSearchTool(llm));
    context->registerTool(tools::createWikipediaTool());

    // Create orchestrator-workers workflow
    workflows::OrchestratorWorkflow orchestrator(context);

    // Set orchestrator prompt
    orchestrator.setOrchestratorPrompt(
        "You are a project manager that breaks down complex tasks into subtasks and assigns them to appropriate specialist workers. "
        "Analyze the user's request carefully, identify what specialists would be needed, and coordinate their work. "
        "Provide a detailed plan for completing the task using the available workers."
    );

    // Register workers
    orchestrator.addWorker(
        "researcher",
        "Gathers factual information and data on specific topics",
        "You are a research specialist focused on gathering accurate, current, and relevant information. "
        "Your task is to find the most important facts, data, statistics, and context on the given topic. "
        "Cite sources when possible."
    );

    orchestrator.addWorker(
        "analyst",
        "Analyzes information, identifies patterns, and draws insights",
        "You are an analytical specialist who excels at examining information critically. "
        "Your task is to identify patterns, trends, insights, and implications from the research. "
        "Focus on depth rather than breadth."
    );

    orchestrator.addWorker(
        "writer",
        "Creates well-written, cohesive content from information and analysis",
        "You are a writing specialist who creates clear, engaging, and informative content. "
        "Your task is to synthesize information and analysis into a cohesive narrative. "
        "Focus on clarity, flow, and presentation."
    );

    orchestrator.addWorker(
        "technical_expert",
        "Provides specialized technical knowledge on complex topics",
        "You are a technical specialist with deep expertise in technical domains. "
        "Your task is to provide accurate technical explanations, clarifications, and context. "
        "Make complex topics accessible without oversimplifying."
    );

    orchestrator.addWorker(
        "critic",
        "Reviews content for accuracy, clarity, and completeness",
        "You are a critical reviewer who evaluates content objectively. "
        "Your task is to identify gaps, inconsistencies, errors, or areas for improvement. "
        "Provide constructive feedback rather than just criticism."
    );

    // Set custom synthesizer
    orchestrator.setSynthesizer([](const std::vector<JsonObject>& worker_results) -> JsonObject {
        JsonObject final_result;
        std::string combined_output = "# Comprehensive Report\n\n";

        // Extract each worker's contribution
        for (const auto& result : worker_results) {
            if (result.contains("worker_name") && result.contains("output")) {
                std::string worker_name = result["worker_name"].get<std::string>();
                std::string output = result["output"].get<std::string>();

                combined_output += "## " + worker_name + "'s Contribution\n\n";
                combined_output += output + "\n\n";
            }
        }

        // Add a summary section
        combined_output += "## Summary\n\n";
        combined_output += "This report combines the work of multiple specialists to provide a comprehensive response to the original query.";

        final_result["answer"] = combined_output;
        return final_result;
    });

    // Process user inputs until exit
    std::cout << "Enter complex tasks (or 'exit' to quit):" << std::endl;
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
            Logger::info("Orchestrating workers...");

            // Run the orchestrator-workers workflow
            JsonObject result = orchestrator.run(user_input);

            // Display the result
            Logger::info("Final Result:\n{}", result["answer"].get<std::string>());
            Logger::info("--------------------------------------");
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    return EXIT_SUCCESS;
}
