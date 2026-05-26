/**
 * @example coroutine_example.cpp
 * @brief Coroutine Example
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <iostream>

using namespace agents;

// Simple callback to print agent steps
void stepCallback(const AutonomousAgent::Step& step) {
    Logger::info("Step: {}", step.description);
    Logger::info("Status: {}", step.status);
    if (step.success) {
        Logger::info("Result: {}", step.result.dump(2));
    } else {
        Logger::error("Failed!");
    }
    Logger::info("--------------------------------------");
}

// Simple callback for human-in-the-loop
bool humanApproval(const std::string& message, const JsonObject& context, std::string& modifications) {
    (void)modifications;
    if (!context.empty()) {
        Logger::info("Context: {}", context.dump(2));
    }
    Logger::info("\n{}", message);
    Logger::info("Approve this step? (y/n): ");
    char response;
    std::cin >> response;
    return (response == 'y' || response == 'Y');
}

// Example coroutine that performs a multi-step task using tools
Task<JsonObject> performResearchTask(std::shared_ptr<Context> context, const std::string& topic) {
    Logger::info("Starting research on topic: {}", topic);

    // Perform a search to get initial information
    auto search_tool = tools::createWebSearchTool(context->getLLM());
    auto search_result = co_await context->executeTool("web_search", {{"query", topic}});

    // Extract key points from the search result
    Logger::info("Extracting key points from search results...");
    auto extract_prompt = "Extract the key points from this search result about " + topic + ":\n\n" + search_result.content;
    auto extract_response = co_await context->chat(extract_prompt);

    // Get more detailed information from Wikipedia
    Logger::info("Getting more information from Wikipedia...");
    auto wiki_tool = tools::createWikipediaTool();
    auto wiki_result = co_await context->executeTool("wikipedia", {{"query", topic}});

    // Combine and summarize all information
    Logger::info("Summarizing all information...");
    auto summarize_prompt = "Synthesize and summarize the following information about " + topic + ":\n\n";
    summarize_prompt += "Key Points:\n" + extract_response.content + "\n\n";
    summarize_prompt += "Wikipedia Information:\n" + wiki_result.content;

    auto summary_response = co_await context->chat(summarize_prompt);

    // Provide a final answer
    JsonObject result = {
        {"topic", topic},
        {"summary", summary_response.content},
        {"search_results", search_result.content},
        {"wiki_results", wiki_result.content}
    };

    Logger::info("Research complete!");
    co_return result;
}

// Example coroutine that generates content in parallel
Task<JsonObject> generateContentInParallel(std::shared_ptr<Context> context, const std::string& topic) {
    Logger::info("Generating content for topic: {}", topic);

    // Create a prompt for the introduction
    std::string intro_prompt = "Write an introduction paragraph for an article about " + topic + ".";

    // Create a prompt for the main body
    std::string body_prompt = "Write three key points about " + topic + " with detailed explanations.";

    // Create a prompt for the conclusion
    std::string conclusion_prompt = "Write a conclusion paragraph for an article about " + topic + ".";

    // Launch all tasks concurrently
    auto intro_task = context->chat(intro_prompt);
    auto body_task = context->chat(body_prompt);
    auto conclusion_task = context->chat(conclusion_prompt);

    // Await all tasks (this happens in parallel)
    auto intro_response = co_await std::move(intro_task);
    auto body_response = co_await std::move(body_task);
    auto conclusion_response = co_await std::move(conclusion_task);

    // Now we have all the pieces, combine them
    std::string article = intro_response.content + "\n\n" + body_response.content + "\n\n" + conclusion_response.content;

    // Create a prompt for a title
    std::string title_prompt = "Create a catchy title for this article:\n\n" + article;
    auto title_response = co_await context->chat(title_prompt);

    // Return the complete article
    JsonObject result = {
        {"title", title_response.content},
        {"introduction", intro_response.content},
        {"body", body_response.content},
        {"conclusion", conclusion_response.content},
        {"full_article", article}
    };

    Logger::info("Content generation complete!");
    co_return result;
}

// Example showing streaming text with coroutines
Task<void> streamText(std::shared_ptr<Context> context, const std::string& prompt) {
    Logger::info("Streaming response for prompt: {}", prompt);

    // Get a streaming generator
    auto generator = context->streamChat(prompt);

    std::cout << "Response: ";

    // Process each chunk as it arrives
    while (auto item = co_await generator.next()) {
        std::string chunk = *item;
        std::cout << chunk << std::flush;
    }
    std::cout << std::endl;

    Logger::info("Streaming complete!");
    co_return;
}

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
    options.temperature = 0.7;
    options.max_tokens = 2048;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Register tools
    context->registerTool(tools::createWikipediaTool());
    context->registerTool(tools::createWebSearchTool(llm));

    // Menu-driven example to demonstrate various coroutines
    while (true) {
        Logger::info("\n========== COROUTINE EXAMPLES ==========");
        Logger::info("1. Run autonomous agent with coroutines");
        Logger::info("2. Perform research with parallel tool use");
        Logger::info("3. Generate content in parallel");
        Logger::info("4. Stream text example");
        Logger::info("5. Exit");
        Logger::info("Enter your choice:");

        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear the newline

        if (choice >= 5 || choice < 1) {
            break;
        }

        // Get topic from user
        std::string topic;
        if (choice >= 1 && choice <= 4) {
            Logger::info("Enter a topic: ");
            std::getline(std::cin, topic);
        }

        try {
            switch (choice) {
                case 1: {
                    // Example 1: Autonomous agent with coroutines
                    Logger::info("Running autonomous agent with coroutines");

                    // Create and configure agent
                    AutonomousAgent agent(context);
                    agent.setAgentPrompt("You are a helpful assistant that can use tools to perform tasks.");
                    agent.setPlanningStrategy(AutonomousAgent::PlanningStrategy::REACT);
                    agent.setStepCallback(stepCallback);

                    // Configure options
                    AutonomousAgent::Options agent_options;
                    agent_options.max_iterations = 3;
                    agent_options.human_feedback_enabled = true;
                    agent_options.human_in_the_loop = humanApproval;
                    agent.setOptions(agent_options);

                    // Initialize
                    agent.init();

                    // Run the agent using coroutines
                    auto result = blockingWait(agent.run(topic));

                    // Display result
                    if (result.contains("answer")) {
                        Logger::info("\nFinal Answer: {}", result["answer"].get<std::string>());
                    } else {
                        Logger::info("\nResult: {}", result.dump(2));
                    }
                    break;
                }
                case 2: {
                    // Example 2: Research task with parallel tool use
                    Logger::info("Performing research with coroutines");
                    auto result = blockingWait(performResearchTask(context, topic));
                    Logger::info("\nResearch Summary: {}", result["summary"].get<std::string>());
                    break;
                }
                case 3: {
                    // Example 3: Generate content in parallel
                    Logger::info("Generating content in parallel");
                    auto result = blockingWait(generateContentInParallel(context, topic));
                    Logger::info("\nTitle: {}", result["title"].get<std::string>());
                    Logger::info("\nFull Article:\n{}", result["full_article"].get<std::string>());
                    break;
                }
                case 4: {
                    // Example 4: Stream text
                    Logger::info("Streaming text example");
                    blockingWait(streamText(context, "Write a short story about " + topic));
                    break;
                }
                default:
                    Logger::error("Invalid choice");
            }
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    return EXIT_SUCCESS;
}