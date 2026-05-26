/**
 * @example evaluator_optimizer_example.cpp
 * @brief Evaluator Optimizer Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/workflows/evaluator_workflow.h>

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
    options.temperature = 0.4;
    options.max_tokens = 2048;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Create evaluator-optimizer workflow
    workflows::EvaluatorWorkflow workflow(context);

    // Set optimizer prompt
    workflow.setOptimizerPrompt(
        "You are an optimizer assistant that produces high-quality responses to user queries. "
        "Your task is to generate the best possible response to the user's query. "
        "If you receive feedback, use it to improve your response."
    );

    // Set evaluator prompt
    workflow.setEvaluatorPrompt(
        "You are an evaluator assistant that critically assesses the quality of responses. "
        "Your task is to provide honest, detailed feedback on the response to help improve it. "
        "Focus on specific areas where the response could be enhanced."
    );

    // Set evaluation criteria
    workflow.setEvaluationCriteria({
        "Accuracy: Is the information provided accurate and factually correct?",
        "Completeness: Does the response address all aspects of the query?",
        "Clarity: Is the response clear, well-organized, and easy to understand?",
        "Relevance: Is the response directly relevant to the query?",
        "Actionability: Does the response provide practical, actionable information where appropriate?"
    });

    // Set maximum iterations
    workflow.setMaxIterations(3);

    // Set minimum acceptable score
    workflow.setMinimumAcceptableScore(0.85);

    // Set custom evaluator and optimizer functions (optional)
    workflow.setEvaluator([](const std::string& input, const std::string& output) -> JsonObject {
        // This is a custom evaluator function that would normally implement
        // specialized evaluation logic, but here we'll let the default LLM-based
        // evaluator do the work by returning an empty result
        (void)input;
        (void)output;
        return JsonObject();
    });

    workflow.setOptimizer([](const std::string& input, const JsonObject& feedback) -> std::string {
        // This is a custom optimizer function that would normally implement
        // specialized optimization logic, but here we'll let the default LLM-based
        // optimizer do the work by returning an empty string
        (void)input;
        (void)feedback;
        return "";
    });

    // Process user inputs until exit
    std::cout << "Enter queries (or 'exit' to quit):" << std::endl;
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
            Logger::info("Starting evaluator-optimizer workflow...");

            // Run the workflow
            JsonObject result = workflow.run(user_input);

            // Display the final result
            Logger::info("\nFinal Response:");
            Logger::info("{}", result["final_response"].get<std::string>());

            // Display evaluation information
            Logger::info("\nEvaluation Information:");
            Logger::info("Iterations: {}", result["iterations"].get<int>());
            Logger::info("Final Score: {}", result["final_score"].get<double>());

            if (result.contains("evaluations")) {
                Logger::info("\nEvaluation History:");
                for (const auto& eval : result["evaluations"]) {
                    Logger::info("Iteration {}: Score = {}", eval["iteration"].get<int>(), eval["score"].get<double>());
                    Logger::info("Feedback: {}", eval["feedback"].get<std::string>());
                    Logger::info("----------");
                }
            }

            Logger::info("--------------------------------------");
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    return EXIT_SUCCESS;
}