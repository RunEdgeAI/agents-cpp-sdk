/**
 * @example parallel_example.cpp
 * @brief Parallel Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/config_loader.h>
#include <agents-cpp/workflows/parallelization_workflow.h>

#include <iostream>

using namespace agents;

int main(int argc, char* argv[]) {
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
        std::cout << ("API key not found. Please:") << std::endl;
        std::cout << ("1. Create a .env file with GEMINI_API_KEY=your_key, or") << std::endl;
        std::cout << ("2. Set the GEMINI_API_KEY environment variable, or") << std::endl;
        std::cout << ("3. Provide an API key as a command line argument") << std::endl;
        return EXIT_FAILURE;
    }

    // Ask the user which parallelization mode to use
    // Sectioning: Breaking a task into independent subtasks run in parallel.
    // Voting: Running the same task multiple times to get diverse outputs.
    std::cout << "Select parallelization mode (1 for SECTIONING, 2 for VOTING): ";
    int mode_choice;
    std::cin >> mode_choice;
    std::cin.ignore(); // Clear the newline

    workflows::ParallelizationWorkflow::Strategy mode = (mode_choice == 2) ?
        workflows::ParallelizationWorkflow::Strategy::VOTING :
        workflows::ParallelizationWorkflow::Strategy::SECTIONING;

    std::cout << "Using mode: " << (mode == workflows::ParallelizationWorkflow::Strategy::VOTING ? "VOTING" : "SECTIONING") << std::endl;

    // Create LLM
    auto llm = createLLM("google", api_key, "gemini-2.0-flash");

    // Configure LLM options
    LLMOptions options;
    options.temperature = 0.7; // Higher temperature for diversity
    options.max_tokens = 2048;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Create parallelization workflow
    workflows::ParallelizationWorkflow parallel(context, mode);

    if (mode == workflows::ParallelizationWorkflow::Strategy::SECTIONING) {
        // Add tasks for sectioning mode
        parallel.addTask(
            "research",
            "You are a research assistant focused on gathering factual information. "
            "Present only verified facts and data, citing sources when possible.",
            [](const std::string& input) -> std::string {
                return "Research task: " + input +
                      "\nFocus on finding the most relevant facts and data points about this topic.";
            },
            [](const std::string& output) -> JsonObject {
                JsonObject result;
                result["research"] = output;
                return result;
            }
        );

        parallel.addTask(
            "analysis",
            "You are an analytical assistant that excels at critical thinking. "
            "Analyze information objectively, identifying patterns, trends, and insights.",
            [](const std::string& input) -> std::string {
                return "Analysis task: " + input +
                      "\nProvide a thoughtful analysis, including implications and significance.";
            },
            [](const std::string& output) -> JsonObject {
                JsonObject result;
                result["analysis"] = output;
                return result;
            }
        );

        parallel.addTask(
            "recommendations",
            "You are a recommendation assistant that provides practical advice. "
            "Suggest actionable steps based on the query.",
            [](const std::string& input) -> std::string {
                return "Recommendation task: " + input +
                      "\nProvide concrete, actionable recommendations related to this topic.";
            },
            [](const std::string& output) -> JsonObject {
                JsonObject result;
                result["recommendations"] = output;
                return result;
            }
        );

        // Set a custom aggregator for sectioning mode
        parallel.setAggregator([](const std::vector<JsonObject>& results) -> JsonObject {
            JsonObject combined;
            std::string research, analysis, recommendations;

            for (const auto& result : results) {
                if (result.contains("research")) {
                    research = result["research"].get<std::string>();
                } else if (result.contains("analysis")) {
                    analysis = result["analysis"].get<std::string>();
                } else if (result.contains("recommendations")) {
                    recommendations = result["recommendations"].get<std::string>();
                }
            }

            combined["answer"] =
                "# Research Findings\n\n" + research +
                "\n\n# Analysis\n\n" + analysis +
                "\n\n# Recommendations\n\n" + recommendations;

            return combined;
        });
    } else {
        // VOTING mode - multiple identical tasks with different parameters
        constexpr uint32_t numVotingAgents = 5u;
        for (uint32_t i = 0; i < numVotingAgents; i++) {
            parallel.addTask(
                "agent_" + std::to_string(i+1),
                "You are assistant " + std::to_string(i+1) + ". "
                "Provide your best answer to the query, thinking independently.",
                [i](const std::string& input) -> std::string {
                    return "Task for agent " + std::to_string(i+1) + ": " + input;
                },
                [](const std::string& output) -> JsonObject {
                    JsonObject result;
                    result["response"] = output;
                    return result;
                }
            );
        }

        // Using default voting aggregator
    }

    // Process user inputs until exit
    std::cout << "Enter queries (or 'exit' to quit):" << std::endl;
    std::string user_input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, user_input);

        if (user_input == "exit" || user_input == "quit" || user_input == "q") {
            break;
        }

        if (user_input.empty()) {
            continue;
        }

        try {
            std::cout << "Running parallel tasks..." << std::endl;

            // Run the parallelization workflow
            JsonObject result = parallel.run(user_input);

            // Display the result
            std::cout << "\nResult:\n" << result["answer"].get<std::string>() << std::endl;
            std::cout << "--------------------------------------" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return EXIT_SUCCESS;
}