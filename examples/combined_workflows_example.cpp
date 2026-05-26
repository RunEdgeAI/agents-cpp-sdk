/**
 * @example combined_workflows_example.cpp
 * @brief End-to-end tour: prompt chaining + parallelization + autonomous agent with tools.
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tool.h>
#include <agents-cpp/workflows/parallelization_workflow.h>
#include <agents-cpp/workflows/prompt_chaining_workflow.h>

using namespace agents;
using namespace agents::workflows;

// Example tool: Calculator
ToolResult calculatorTool(const JsonObject& params) {
    try {
        if (params.contains("expression")) {
            std::string expr = params["expression"];
            // Very simple calculator for demo purposes
            // In a real-world scenario, you'd use a proper expression evaluator
            double result = 0.0;

            // Just a dummy implementation for demo purposes
            if (expr == "1+1") {
                result = 2.0;
            } else if (expr == "2*3") {
                result = 6.0;
            } else {
                // Default response
                result = 42.0;
            }

            return {
                true,
                "Calculated result: " + std::to_string(result),
                {{"result", result}}
            };
        } else {
            return {
                false,
                "Missing expression parameter",
                {{"error", "Missing expression parameter"}}
            };
        }
    } catch (const std::exception& e) {
        return {
            false,
            "Error calculating result: " + std::string(e.what()),
            {{"error", e.what()}}
        };
    }
}

// Example tool: Weather
ToolResult weatherTool(const JsonObject& params) {
    try {
        if (params.contains("location")) {
            std::string location = params["location"];

            // Just a dummy implementation for demo purposes
            std::string weather = "sunny";
            double temperature = 22.0;

            return {
                true,
                "Weather in " + location + ": " + weather + ", " + std::to_string(temperature) + "°C",
                {
                    {"location", location},
                    {"weather", weather},
                    {"temperature", temperature}
                }
            };
        } else {
            return {
                false,
                "Missing location parameter",
                {{"error", "Missing location parameter"}}
            };
        }
    } catch (const std::exception& e) {
        return {
            false,
            "Error getting weather: " + std::string(e.what()),
            {{"error", e.what()}}
        };
    }
}

int main(int argc, char* argv[]) {
    // Set up logging
    Logger::setLevel(Logger::Level::INFO);

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

    try {
        // Create LLM interface
        auto llm = createLLM("google", api_key, "gemini-2.0-flash");

        // Set up options
        LLMOptions options;
        options.temperature = 0.7;
        options.max_tokens = 1000;
        llm->setOptions(options);

        // Create tools
        auto calculator = createTool(
            "calculator",
            "Calculate mathematical expressions",
            {
                {"expression", "The mathematical expression to calculate", "string", true}
            },
            calculatorTool
        );

        auto weather = createTool(
            "weather",
            "Get weather information for a location",
            {
                {"location", "The location to get weather for", "string", true}
            },
            weatherTool
        );

        // Create agent context
        auto context = std::make_shared<Context>();
        context->setLLM(llm);
        context->registerTool(calculator);
        context->registerTool(weather);

        // Example 1: Using the prompt chaining workflow
        Logger::info("\n=== Example 1: Prompt Chaining Workflow ===\n\n");

        auto chaining_workflow = std::make_shared<PromptChainingWorkflow>(context);

        // Add steps to the workflow
        chaining_workflow->addStep(
            "brainstorm",
            "Brainstorm 3 creative ideas for a short story about space exploration. Return them as a JSON array."
        );

        chaining_workflow->addStep(
            "select",
            "From these ideas, select the most interesting one and explain why you chose it:\n{{response}}"
        );

        chaining_workflow->addStep(
            "outline",
            "Create a brief outline for a story based on this idea:\n{{response}}"
        );

        // Initialize and execute the workflow
        auto result = chaining_workflow->run();

        Logger::info("Prompt chaining result: {}", result.dump(2));

        // Example 2: Using the parallelization workflow
        Logger::info("\n=== Example 2: Parallelization Workflow (Sectioning) ===\n\n");

        auto parallel_workflow = std::make_shared<ParallelizationWorkflow>(
            context, ParallelizationWorkflow::Strategy::SECTIONING
        );

        // Add tasks to the workflow
        parallel_workflow->addTask(
            "characters",
            "Create 2 interesting characters for a sci-fi story set on Mars."
        );

        parallel_workflow->addTask(
            "setting",
            "Describe the environment and setting of a Mars colony in the year 2150."
        );

        parallel_workflow->addTask(
            "plot",
            "Create a plot outline for a mystery story set on Mars."
        );

        // Initialize and execute the workflow
        parallel_workflow->init();
        result = parallel_workflow->run();

        Logger::info("Parallelization result: {}", result.dump(2));

        // Example 3: Using the autonomous agent with tools
        Logger::info("\n=== Example 3: Autonomous Agent with Tools ===\n\n");

        auto agent = std::make_shared<AutonomousAgent>(context);

        // Set agent prompt
        agent->setAgentPrompt(
            "You are a helpful assistant that can answer questions and use tools to get information. "
            "When using tools, make sure to include all necessary parameters."
        );

        // Set options
        Agent::Options agent_options;
        agent_options.max_iterations = 5;
        agent_options.human_feedback_enabled = false;
        agent->setOptions(agent_options);

        // Register status callback
        agent->setStatusCallback([](const std::string& status) {
            Logger::info("Agent status: {}", status);
        });

        // Initialize and run the agent
        agent->init();

        // Run the agent with multiple tasks
        std::vector<std::string> tasks = {
            "What is 1+1?",
            "What's the weather like in New York?",
            "Tell me a short story about a robot learning to feel emotions."
        };

        for (const auto& task : tasks) {
            Logger::info("Task: {}", task);
            result = blockingWait(agent->run(task));
            Logger::info("Result: {}", result.dump(2));

            // Small delay between tasks
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        Logger::error("Error: {}", e.what());
        return EXIT_FAILURE;
    }
}