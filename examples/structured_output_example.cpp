/**
 * @file structured_output_example.cpp
 * @brief Example demonstrating structured output with LLM
 * @version 0.1
 * @date 2025-01-27
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#include <agents-cpp/config_loader.h>
#include <agents-cpp/llm_interface.h>
#include <agents-cpp/logger.h>

#include <iostream>

using namespace agents;

int main(int argc, char* argv[]) {
    // Initialize logger
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

    // Create Google LLM instance
    auto llm = createLLM("google", api_key, "gemini-2.0-flash");

    // Example 1: Basic structured output for a simple task
    {
        std::cout << "\n=== Example 1: Basic Structured Output ===" << std::endl;

        // Define a schema for extracting recipe information
        JsonObject recipe_schema = {
            {"type", "object"},
            {"properties", {
                {"recipe_name", {
                    {"type", "STRING"},
                    {"description", "Name of the recipe"}
                }},
                {"ingredients", {
                    {"type", "ARRAY"},
                    {"items", {
                        {"type", "OBJECT"},
                        {"properties", {
                            {"ingredient", {{"type", "STRING"}}},
                            {"quantity", {{"type", "STRING"}}},
                            {"unitOfMeasurement", {{"type", "STRING"}}}
                        }},
                        {"required", {"ingredient", "quantity", "unitOfMeasurement"}},
                        {"propertyOrdering", {"ingredient", "quantity", "unitOfMeasurement"}}
                    }}
                }},
                {"cooking_time", {
                    {"type", "STRING"},
                    {"description", "Estimated cooking time"}
                }}
            }},
            {"required", {"recipe_name", "ingredients"}},
            {"propertyOrdering", {"recipe_name", "ingredients", "cooking_time"}}
        };

        // Configure LLM options for structured output
        LLMOptions options;
        options.response_schema = recipe_schema;
        options.response_mime_type = "application/json";
        options.temperature = 0.1; // Lower temperature for more consistent structured output

        // Debug: Print the schema being sent
        std::cout << "Debug: Schema being sent:" << std::endl;
        std::cout << recipe_schema.dump(2) << std::endl;

        llm->setOptions(options);

        // Send a prompt
        std::string prompt = "Give me a simple chocolate chip cookie recipe with 5 ingredients.";
        auto response = llm->chat(prompt);

        std::cout << "Prompt: " << prompt << std::endl;
        std::cout << "Structured Response: " << response.content << std::endl;
    }

    // Example 2: Enum-based structured output
    {
        std::cout << "\n=== Example 2: Enum-based Structured Output ===" << std::endl;

        // Define an enum schema for classification
        JsonObject classification_schema = {
            {"type", "STRING"},
            {"enum", {"positive", "negative", "neutral"}}
        };

        // Configure LLM options for enum output
        LLMOptions options;
        options.response_schema = classification_schema;
        options.response_mime_type = "text/x.enum";
        options.temperature = 0.1;

        llm->setOptions(options);

        // Send a prompt for sentiment analysis
        std::string prompt = "Classify the sentiment of these statements: 'I love this product', 'This is terrible', 'It's okay'";
        auto response = llm->chat(prompt);

        std::cout << "Prompt: " << prompt << std::endl;
        std::cout << "Enum Response: " << response.content << std::endl;
    }

    // Example 3: Custom complex schema
    {
        std::cout << "\n=== Example 3: Custom Complex Schema ===" << std::endl;

        // Define a complex schema for a customer support ticket
        JsonObject ticket_schema = {
            {"type", "OBJECT"},
            {"properties", {
                {"priority", {
                    {"type", "STRING"},
                    {"enum", {"low", "medium", "high", "critical"}}
                }},
                {"category", {
                    {"type", "STRING"},
                    {"enum", {"technical", "billing", "feature_request", "bug_report"}}
                }},
                {"summary", {
                    {"type", "STRING"},
                    {"description", "Brief summary of the issue"}
                }},
                {"description", {
                    {"type", "STRING"},
                    {"description", "Detailed description of the issue"}
                }},
                {"suggested_solution", {
                    {"type", "STRING"},
                    {"description", "Proposed solution if available"}
                }},
                {"tags", {
                    {"type", "ARRAY"},
                    {"items", {{"type", "STRING"}}}
                }}
            }},
            {"required", {"priority", "category", "summary", "description"}},
            {"propertyOrdering", {"priority", "category", "summary", "description", "suggested_solution", "tags"}}
        };

        // Configure LLM options
        LLMOptions options;
        options.response_schema = ticket_schema;
        options.response_mime_type = "application/json";
        options.temperature = 0.1;

        llm->setOptions(options);

        // Send a prompt for ticket creation
        std::string prompt = "Create a support ticket for a user who can't log into their account and is getting an 'invalid credentials' error.";
        auto response = llm->chat(prompt);

        std::cout << "Prompt: " << prompt << std::endl;
        std::cout << "Complex Schema Response: " << response.content << std::endl;
    }

    std::cout << "\n=== Structured Output Examples Complete ===" << std::endl;
    return EXIT_SUCCESS;
}
