/**
 * @example in-car-ai.cpp
 * @brief In-Car AI Demo
 * @version 0.2
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 */
#include <agents-cpp/agents/voice_agent.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/http_client.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>

using namespace agents;

// Shared state for navigation data (written by navigate tool, read by UI)
static std::mutex nav_mutex;
static std::string nav_json;

// Shared state for AC controls (written by AC tool, read by UI)
static std::mutex ac_mutex;
static std::string ac_json;

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
        [context, &config](const JsonObject& params) -> ToolResult {
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
            std::string directions = "Directions: ";

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

            // Store navigation data for UI consumption
            {
                std::lock_guard<std::mutex> lock(nav_mutex);
                nav_json = resp.text;
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
            location["latitude"] = 37.8052;
            location["longitude"] = -122.432091;
            if (address) {
                location["address"] = "2 Marina Blvd, San Francisco, CA";
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
            {"mode", "The mode of operation: cool, heat, or auto", "string", false}
        },
        [context](const JsonObject& params) -> ToolResult {
            int temperature = params["temperature"];
            int fan_speed = params.contains("fan_speed") ? params["fan_speed"].get<int>() : 3;
            std::string mode = params.contains("mode") ? params["mode"].get<std::string>() : "auto";

            std::string result = "Setting air conditioner to " + std::to_string(temperature) +
                                 " degrees, fan speed " + std::to_string(fan_speed) +
                                 ", mode " + mode + ".";

            // Store AC state for UI
            {
                std::lock_guard<std::mutex> lock(ac_mutex);
                JsonObject ac_state;
                ac_state["temperature"] = temperature;
                ac_state["fan_speed"] = fan_speed;
                ac_state["mode"] = mode;
                ac_json = ac_state.dump();
            }

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
    VoiceAgent::Config cfg;
    cfg.agent_endpoint = "http://127.0.0.1:8080/agent";
    cfg.stt_endpoint = "http://127.0.0.1:8888/";
    cfg.tts_endpoint = "http://127.0.0.1:9999/";
    cfg.api_key = config.get("EDGEAI_API_KEY");

    VoiceAgent agent(context, cfg);

    // Set the agent prompt (this extends the context system prompt for the agent)
    agent.setAgentPrompt(config.get("SYSTEM_PROMPT"));

    // Initialize the agent
    agent.init();

    // Run the in-car UI in a separate thread
    httplib::Server svr;

    // Shared state for streaming partial responses to UI
    std::mutex partial_mutex;
    std::string current_partial;

    // Serve the in-car UI
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = Utils::loadHtmlFile("sample_media/ui/in-car.html");
        res.set_content(html, "text/html");
    });

    // Trigger listen via UI
    svr.Post("/api/trigger", [&agent](const httplib::Request&, httplib::Response& res) {
        Logger::info("[Backend] Agent triggered via Web UI!");
        agent.listen();
        res.set_content("OK", "text/plain");
    });

    // Transcript API (chat + voice status)
    svr.Get("/api/transcript", [&agent, &partial_mutex, &current_partial](const httplib::Request&, httplib::Response& res) {
        auto state = agent.getState();
        std::string state_str;
        switch (state) {
            case VoiceAgent::State::LISTENING: state_str = "1"; break;
            case VoiceAgent::State::PROCESSING: state_str = "2"; break;
            case VoiceAgent::State::SPEAKING: state_str = "3"; break;
            default: state_str = "0"; break;
        }

        std::string summary;
        auto ctx = agent.getContext();
        if (ctx && ctx->getMemory()) {
            summary = ctx->getMemory()->getConversationSummary();
        }

        {
            std::lock_guard<std::mutex> lock(partial_mutex);
            if (!current_partial.empty()) {
                summary += "Assistant: " + current_partial + "\n\n";
            }
        }

        res.set_header("X-Voice-Status", state_str);
        res.set_content(summary, "text/plain");
    });

    // Navigation data API (polled by UI to display turn-by-turn directions)
    svr.Get("/api/navigation", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(nav_mutex);
        if (nav_json.empty()) {
            res.status = 204;
            return;
        }
        res.set_content(nav_json, "application/json");
    });

    // Music "now playing" API (calls Apple Music MCP tool directly)
    svr.Get("/api/music", [&context](const httplib::Request&, httplib::Response& res) {
        auto tool = context->getTool("itunes_current_song");
        if (!tool) {
            res.status = 204;
            return;
        }
        try {
            auto result = tool->execute({});
            if (result.success && !result.content.empty()) {
                // Filter out "nothing playing" responses from Apple Music
                std::string lower = result.content;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("nothing") != std::string::npos ||
                    lower.find("not playing") != std::string::npos ||
                    lower.find("no track") != std::string::npos ||
                    lower.find("stopped") != std::string::npos ||
                    lower.find("error") != std::string::npos ||
                    lower.find("can't get") != std::string::npos) {
                    res.status = 204;
                } else {
                    res.set_content(result.content, "text/plain");
                }
            } else {
                res.status = 204;
            }
        } catch (...) {
            res.status = 204;
        }
    });

    // Climate / AC controls API
    svr.Get("/api/climate", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(ac_mutex);
        if (ac_json.empty()) {
            res.status = 204;
            return;
        }
        res.set_content(ac_json, "application/json");
    });

    // Stop API
    svr.Post("/api/stop", [&agent](const httplib::Request&, httplib::Response& res) {
        Logger::info("[Backend] Stop requested via Web UI!");
        agent.stop();
        res.set_content("OK", "text/plain");
    });

    // TTS voice list proxy (avoids CORS since TTS is on a different port)
    svr.Get("/api/voices", [&cfg](const httplib::Request&, httplib::Response& res) {
        try {
            std::string voices_url = cfg.tts_endpoint + "voices";
            std::map<std::string, std::string> empty_headers;
            HTTPClient::Response tts_resp = HTTPClient::get(voices_url, empty_headers);
            if (tts_resp.status_code == 200 && !tts_resp.text.empty()) {
                res.set_content(tts_resp.text, "application/json");
            } else {
                res.status = tts_resp.status_code > 0 ? tts_resp.status_code : 502;
            }
        } catch (...) {
            res.status = 502;
        }
    });

    // TTS voice switch proxy
    svr.Post("/api/loadVoice", [&cfg](const httplib::Request& req, httplib::Response& res) {
        try {
            std::map<std::string, std::string> headers = {{"Content-Type", "text/plain"}};
            if (!cfg.api_key.empty()) {
                headers["Authorization"] = "Bearer " + cfg.api_key;
            }
            HTTPClient::Response tts_resp = HTTPClient::post(cfg.tts_endpoint + "loadVoice", headers, req.body);
            res.status = tts_resp.status_code;
            res.set_content(tts_resp.text, "text/plain");
        } catch (...) {
            res.status = 502;
        }
    });

    // Clear context/memory API
    svr.Post("/api/clear", [&agent](const httplib::Request&, httplib::Response& res) {
        Logger::info("[Backend] Clear context requested via Web UI!");
        auto ctx = agent.getContext();
        if (ctx && ctx->getMemory()) {
            ctx->getMemory()->clear();
        }
        res.set_content("OK", "text/plain");
    });

    // Hook callbacks for streaming partial responses to the UI
    agent.cb_.onPartialResponse = [&partial_mutex, &current_partial](const std::string& chunk) {
        std::lock_guard<std::mutex> lock(partial_mutex);
        current_partial += chunk;
    };

    agent.cb_.onFinalResponse = [&partial_mutex, &current_partial](const std::string& text) {
        (void)text;
        std::lock_guard<std::mutex> lock(partial_mutex);
        current_partial.clear();
    };

    std::thread ui_thread([&svr]() {
        Logger::info("In-Car UI server listening on http://127.0.0.1:9000");
        svr.listen("0.0.0.0", 9000);
    });

    // Get user input
    Logger::info("Press 'a' and speak a question (or 'exit' to quit):");

    std::string user_input;
    while (true) {
        Logger::info("\n> ");
        std::getline(std::cin, user_input);

        if (user_input.empty()) {
            continue;
        }

        if (user_input == "exit" || user_input == "quit" || user_input == "q") {
            break;
        }

        if (user_input == "a") {
            agent.listen();
            continue;
        }

        // Run the agent
        agent.runAsync(user_input, [&](const JsonObject& jsonPartial) {
            if (!jsonPartial.empty() && jsonPartial.contains("part") && !jsonPartial["part"].is_null()) {
                auto part = jsonPartial["part"].get<std::string>();
                std::cout << part << std::flush;
            }
            // Note: Final result is also sent via this callback with "answer"
            if (!jsonPartial.empty() && jsonPartial.contains("answer")) {
                std::cout << std::endl;
            }
        });
    }

    // Cleanup
    agent.stop();
    svr.stop();
    ui_thread.join();

    return EXIT_SUCCESS;
}
