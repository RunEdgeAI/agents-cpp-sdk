/**
 * @example autonomous_agent_example.cpp
 * @brief Autonomous Agent Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <chrono>
#include <iostream>

using namespace agents;

// Custom callback to print detailed agent steps
void detailedStepCallback(const AutonomousAgent::Step& step) {
    Logger::info("\n=== STEP ===");
    Logger::info("Description: {}", step.description);
    Logger::info("Status: {}", step.status);

    if (step.success) {
        Logger::info("\nResult: {}", step.result.dump(2));
    } else {
        Logger::error("\nFailed!");
    }

    Logger::info("------------------------------------");
}

// Custom callback for human-in-the-loop
bool detailedHumanApproval(const std::string& message, const JsonObject& context, std::string& modifications) {
    if (!context.empty()) {
        Logger::info("\nContext Information:");
        Logger::info("{}", context.dump(2));
    }

    Logger::info("🔔 HUMAN APPROVAL REQUIRED 🔔");
    Logger::info("{}", message);
    Logger::info("Approve this step? (y/n/m - y: approve, n: reject, m: modify): ");
    char response;
    std::cin >> response;
    std::cin.ignore(); // Clear the newline

    if (response == 'm' || response == 'M') {
        Logger::info("Enter your modifications or instructions: ");
        std::string user_modifications;
        std::getline(std::cin, user_modifications);

        // Set modifications output parameter
        modifications = user_modifications;

        // Return true to continue with modifications
        Logger::info("Continuing with your modifications...");
        return true;
    }

    return (response == 'y' || response == 'Y');
}

int main() {
    // Initialize the logger
    Logger::init(Logger::Level::INFO);

    // Get model choice from user
    Logger::info("Select LLM provider (1 for OpenAI, 2 for Anthropic, 3 for Google): ");
    int provider_choice;
    std::cin >> provider_choice;
    std::cin.ignore(); // Clear the newline

    // Create LLM based on user choice
    std::shared_ptr<LLMInterface> llm;

    // Check if we have any API keys configured
    auto& config = ConfigLoader::getInstance();

    // Proceed based on provider choice and API key in the environment
    try {
        if (provider_choice == 1) {
            llm = createLLM("openai", config.get("OPENAI_API_KEY"), "gpt-4o");
        } else if (provider_choice == 2) {
            llm = createLLM("anthropic", config.get("ANTHROPIC_API_KEY"), "claude-sonnet-4-5");
        } else if (provider_choice == 3) {
            llm = createLLM("google", config.get("GEMINI_API_KEY"), "gemini-2.5-flash");
        } else {
            Logger::error("Invalid provider choice.");
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        Logger::error("Error creating LLM: {}", e.what());
        Logger::error("Please ensure the appropriate API key is set in the environment.");
        return EXIT_FAILURE;
    }

    // Configure LLM options
    LLMOptions options;
    options.temperature = 0.2;
    options.max_tokens = 4096;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Set system prompt for the context
    context->setSystemPrompt(
        "You are a helpful, autonomous assistant with access to tools. "
        "You can use these tools to accomplish tasks for the user. "
        "Think step by step and be thorough in your approach."
    );

    // Register tools from tool registry
    auto registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // Create a custom tool
    auto summarize_tool = createTool(
        "summarize",
        "Summarizes a long piece of text into a concise summary",
        {
            {"text", "The text to summarize", "string", true},
            {"max_length", "Maximum length of summary in words", "integer", false}
        },
        [context](const JsonObject& params) -> ToolResult {
            std::string text = params["text"];
            int max_length = params.contains("max_length") ? params["max_length"].get<int>() : 100;

            // Create a specific context for summarization
            auto summary_context = std::make_shared<Context>(*context);
            summary_context->setSystemPrompt(
                "You are a summarization assistant. Your task is to create concise, accurate summaries "
                "that capture the main points of the provided text."
            );

            std::string prompt = "Summarize the following text in no more than " +
                            std::to_string(max_length) + " words:\n\n" + text;

            LLMResponse llm_response = summary_context->getLLM()->chat(prompt);
            std::string summary = llm_response.content;

            return ToolResult{
                true,
                summary,
                {{"summary", prompt + "\n\n" + summary}}
            };
        }
    );
    context->registerTool(summarize_tool);

    // Allow the user to choose planning strategy
    Logger::info("Select planning strategy:");
    Logger::info("1. ReAct (Open-ended, evolving task)");
    Logger::info("2. Plan-and-Execute (Complex, structured task)");
    Logger::info("Choice: ");
    int strategy_choice;
    std::cin >> strategy_choice;
    std::cin.ignore(); // Clear the newline

    // Create the agent
    AutonomousAgent agent(context);

    // Set planning strategy based on user choice
    AutonomousAgent::PlanningStrategy strategy;
    switch (strategy_choice) {
        case 1:
            strategy = AutonomousAgent::PlanningStrategy::REACT;
            break;
        case 2:
            strategy = AutonomousAgent::PlanningStrategy::PLAN_AND_EXECUTE;
            break;
        default:
            Logger::error("Invalid strategy choice.");
            return EXIT_FAILURE;
    }
    agent.setPlanningStrategy(strategy);

    // Set the agent prompt (this extends the context system prompt for the agent)
    agent.setAgentPrompt(
        "You are an advanced autonomous assistant capable of using tools to help users "
        "accomplish their tasks. You break down complex problems into manageable steps "
        "and execute them systematically. Always provide clear explanations of your "
        "reasoning and approach."
    );

    // Set up options
    AutonomousAgent::Options agent_options;
    agent_options.max_iterations = 15;

    // Ask user if they want human-in-the-loop mode
    Logger::info("Enable human-in-the-loop mode? (y/n): ");
    char human_loop_choice;
    std::cin >> human_loop_choice;
    std::cin.ignore(); // Clear the newline

    agent_options.human_feedback_enabled = (human_loop_choice == 'y' || human_loop_choice == 'Y');
    if (agent_options.human_feedback_enabled) {
        agent_options.human_in_the_loop = detailedHumanApproval;
    }

    agent.setOptions(agent_options);

    // Set up callbacks
    agent.setStepCallback(detailedStepCallback);

    // Initialize the agent
    agent.init();

    // Get user input
    Logger::info("==================================================");
    Logger::info("                AUTONOMOUS AGENT                  ");
    Logger::info("==================================================");
    Logger::info("Enter a question or task for the agent (or 'exit' to quit):");

    std::string user_input;
    while (true) {
        Logger::info("\n> ");
        std::getline(std::cin, user_input);

        if (user_input == "exit" || user_input == "quit" || user_input == "q") {
            break;
        }

        if (user_input.empty()) {
            continue;
        }

        try {
            // Start a timer to measure execution time
            auto start_time = std::chrono::high_resolution_clock::now();

            // Run the agent
            JsonObject result = blockingWait(agent.run(user_input));

            // End timer
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

            // Display the final result
            Logger::info("==================================================");
            Logger::info("                  FINAL RESULT                    ");
            Logger::info("==================================================");
            Logger::info("{}", result["answer"].get<std::string>());

            // Display completion statistics
            if (result.contains("steps")) {
                Logger::info("--------------------------------------------------");
                Logger::info("Task completed in {} seconds", duration);
                Logger::info("Total steps: {}", result["steps"].get<JsonArray>().size());
            }

            if (result.contains("tool_calls")) {
                Logger::info("Tool calls: {}", result["tool_calls"].get<int>());
            }

            Logger::info("==================================================");
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    return EXIT_SUCCESS;
}
