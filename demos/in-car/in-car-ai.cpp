/**
 * @example in-car-ai.cpp
 * @brief In-Car AI Demo
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 */
#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/http_client.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <chrono>
#include <iostream>

using namespace agents;

int main() {
    // Initialize the logger
    Logger::init(Logger::Level::INFO);

    // Create LLM based on user choice
    std::shared_ptr<LLMInterface> llm;

    // Check if we have any API keys configured
    auto& config = ConfigLoader::getInstance();

    // Use LLM based API key avalable
    try {
        llm = createLLM(config.get("PROVIDER"), config.get("API_KEY"), config.get("MODEL"));
    } catch (const std::exception& e) {
        Logger::error("Error creating LLM: {}", e.what());
        Logger::error("Please ensure the appropriate API key is set in the environment.");
        return EXIT_FAILURE;
    }

    // Configure LLM options
    LLMOptions options;
    options.temperature = 0.2;
    options.max_tokens = 1024;
    llm->setOptions(options);

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Register tools from tool registry
    auto registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // Create a custom tools
    auto navigation_tool = createTool(
        "navigate",
        "Provides turn-by-turn navigation instructions to a specified destination given both start and end addresses or start and end latitude/longitude. "
        "Never provide both addresses and lat/longs in the same request. "
        "Use this over web search for directions.",
        {
            {"start_address", "The starting address", "string", false},
            {"end_address", "The destination address", "string", false},
            {"start_long", "The starting longitude", "number", false},
            {"start_lat", "The starting latitude", "number", false},
            {"end_long", "The destination longitude", "number", false},
            {"end_lat", "The destination latitude", "number", false},
        },
        [context, config](const JsonObject& params) -> ToolResult {
            std::string api_key = config.get("MAPS_API_KEY");
            const std::string url = "https://maps.googleapis.com/maps/api/directions/json";

            auto build_location = [&](const std::string& prefix) -> std::string {
                // Case 1: lat/lng provided
                if (params.contains(prefix + "_lat") && params[prefix + "_lat"] != nullptr &&
                    params.contains(prefix + "_long") && params[prefix + "_long"] != nullptr) {
                    double lat  = params[prefix + "_lat"].get<double>();
                    double lng  = params[prefix + "_long"].get<double>();
                    return std::to_string(lat) + "," + std::to_string(lng); // Google wants "lat,lng"
                }

                // Case 2: freeform address provided
                if (params.contains(prefix + "_address") && params[prefix + "_address"] != nullptr) {
                    return params[prefix + "_address"].get<std::string>();
                }

                // Case 3: missing or invalid
                Logger::error("Missing or invalid {} location parameters", params.dump(2));
                return "";
            };

            auto strip_html_tags = [&](const std::string& html) -> std::string {
                static const std::regex tag_re("<[^>]*>");
                return std::regex_replace(html, tag_re, "");
            };

            std::string origin = build_location("start");
            std::string destination = build_location("end");

            httplib::Params nav_params = {
                {"origin", origin},
                {"destination", destination},
                {"key", api_key}
            };

            std::map<std::string, std::string> headers = {
                {"Accept", "application/json"},
                {"Content-Type", "application/json; charset=utf-8"}
            };

            HTTPClient::Response resp = HTTPClient::get(url, nav_params, headers);
            std::string directions;

            // process response
            if (resp.status_code == 200)
            {
                JsonObject j = JsonObject::parse(resp.text);
                for (const auto& route : j["routes"]) {
                    for (const auto& leg : route["legs"]) {
                        for (const auto& step : leg["steps"]) {
                            std::string html_instr = step["html_instructions"].get<std::string>();
                            directions += strip_html_tags(html_instr) + ". ";
                        }
                    }
                }
            } else {
                directions = "Failed to fetch navigation data";
            }

            return ToolResult{
                true,
                directions,
                {{"navigation_data", JsonObject::parse(resp.text)}}
            };
        }
    );

    auto location_tool = createTool(
        "get_location",
        "Fetch current location. Used for navigation and location-based queries.",
        {
            {"address", "Whether to return the full address or just coordinates", "boolean", true},
            {"latitude", "The latitude of the location", "number", false},
            {"longitude", "The longitude of the location", "number", false}
        },
        [context](const JsonObject& params) -> ToolResult {
            bool address = params.contains("address") ? params["address"].get<bool>() : false;

            // Simulate fetching location
            JsonObject location;
            location["latitude"] = 37.7749;
            location["longitude"] = -122.4194;
            if (address) {
                location["address"] = "1 Infinite Loop, Cupertino, CA";
            }

            std::string result = "Current location is (" +
                                 std::to_string(location["latitude"].get<double>()) + ", " +
                                 std::to_string(location["longitude"].get<double>()) + ")";;
            if (address) {
                result += " - " + location["address"].get<std::string>();
            }

            return ToolResult{
                true,
                result,
                {{"location", location}}
            };
        }
    );

    auto ac_controls_tool = createTool(
        "air_conditioner_control",
        "Controls the vehicle's air conditioning system to adjust temperature and airflow.",
        {
            {"temperature", "The desired temperature setting in fahrenheit", "integer", true},
            {"fan_speed", "The speed of the fan (1-5)", "integer", false},
            {"mode", "The mode of operation (e.g., cool, heat, auto)", "string", false}
        },
        [context](const JsonObject& params) -> ToolResult {
            int temperature = params["temperature"];
            int fan_speed = params.contains("fan_speed") ? params["fan_speed"].get<int>() : 3;
            std::string mode = params.contains("mode") ? params["mode"].get<std::string>() : "auto";

            std::string result = "Setting air conditioner to " + std::to_string(temperature) +
                                 " degrees, fan speed " + std::to_string(fan_speed) +
                                 ", mode " + mode + ".";

            return ToolResult{
                true,
                result,
                {{"ac_controls", result}}
            };
        }
    );

    // Register custom tools
    context->registerTool(ac_controls_tool);
    context->registerTool(location_tool);
    context->registerTool(navigation_tool);

    // Create the agent
    AutonomousAgent agent(context);
    agent.setPlanningStrategy(AutonomousAgent::PlanningStrategy::REACT);

    // Set the agent prompt (this extends the context system prompt for the agent)
    agent.setAgentPrompt(
        "You are an advanced autonomous in-car assistant capable of using tools to help users "
        "accomplish their tasks. You break down complex problems into manageable steps "
        "and execute them systematically."
    );

    // Set up options
    AutonomousAgent::Options agent_options;
    agent_options.max_iterations = 15;
    agent.setOptions(agent_options);

    // Initialize the agent
    agent.init();

    // Get user input
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
            Logger::info("\n--------------------------------------------------");
            Logger::info("Task completed in {} seconds", duration);
            Logger::info("==================================================");
        } catch (const std::exception& e) {
            Logger::error("Error: {}", e.what());
        }
    }

    return EXIT_SUCCESS;
}