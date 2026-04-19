/**
 * @example in-car-ai.cpp
 * @brief In-Car AI Demo
 * @version 0.3
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */
#include "ui.hpp"

#include <agents-cpp/config_loader.h>
#include <agents-cpp/tools/tool_registry.h>

#include <iostream>
#include <regex>

using namespace agents;

int main() {
    Logger::init(Logger::Level::INFO);

    // ── Config + LLM ──────────────────────────────────────────────────────────
    auto& config = ConfigLoader::getInstance();
    std::shared_ptr<LLMInterface> llm;
    try {
        llm = createLLM(config.get("PROVIDER"), config.get("API_KEY"), config.get("MODEL"));
    } catch (const std::exception& e) {
        Logger::error("Error creating LLM: {}", e.what());
        return EXIT_FAILURE;
    }

    // ── Context ───────────────────────────────────────────────────────────────
    auto context = std::make_shared<Context>(llm);
    context->setTrustLevel(TrustLevel::FULL);

    // Persona, behavior rules, MCP servers, and skills loaded from markdown files.
    // demos/in-car/.agents/ is scanned first, cwd/.agents/, then root .agents/.
    context->loadWorkspaceConfig("demos/in-car/.agents");

    // ── Tools ─────────────────────────────────────────────────────────────────
    tools::ToolRegistry registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // Shared vehicle state — written by tools below, read by the UI server
    VehicleState vehicle;

    auto navigation_tool = createTool(
        "navigate",
        "Provides turn-by-turn navigation to a destination. "
        "Accepts start/end as addresses or lat/lon pairs (never both). "
        "Prefer this over web search for directions.",
        {
            {"start_address", "Starting address",       "string", false},
            {"end_address",   "Destination address",    "string", false},
            {"start_long",    "Starting longitude",     "number", false},
            {"start_lat",     "Starting latitude",      "number", false},
            {"end_long",      "Destination longitude",  "number", false},
            {"end_lat",       "Destination latitude",   "number", false},
        },
        [&config, &vehicle](const JsonObject& params) -> ToolResult {
            const std::string url = "https://maps.googleapis.com/maps/api/directions/json";

            auto build_location = [&](const std::string& prefix) -> std::string {
                if (params.contains(prefix + "_lat") && params[prefix + "_lat"] != nullptr &&
                    params.contains(prefix + "_long") && params[prefix + "_long"] != nullptr)
                    return std::to_string(params[prefix + "_lat"].get<double>()) + "," +
                           std::to_string(params[prefix + "_long"].get<double>());
                if (params.contains(prefix + "_address") && params[prefix + "_address"] != nullptr)
                    return params[prefix + "_address"].get<std::string>();
                return "";
            };

            static const std::regex tag_re("<[^>]*>");
            auto strip_html = [](const std::string& s) {
                return std::regex_replace(s, std::regex("<[^>]*>"), "");
            };

            httplib::Params nav_params = {
                {"origin",      build_location("start")},
                {"destination", build_location("end")},
                {"key",         config.get("MAPS_API_KEY")}
            };
            std::map<std::string, std::string> headers = {{"Accept", "application/json"}};
            HTTPClient::Response resp = HTTPClient::get(url, nav_params, headers);

            std::string directions = "Directions: ";
            bool ret = true;
            if (resp.status_code == 200) {
                JsonObject j = JsonObject::parse(resp.text);
                for (const auto& route : j["routes"])
                    for (const auto& leg : route["legs"])
                        for (const auto& step : leg["steps"])
                            directions += strip_html(step["html_instructions"].get<std::string>()) + ". ";
            } else {
                ret = false;
                directions = "Failed to fetch navigation data";
            }

            { std::lock_guard<std::mutex> lk(vehicle.nav_mutex); vehicle.nav_json = resp.text; }
            return ToolResult{ret, directions, {{"navigation_data", JsonObject::parse(resp.text)}}};
        }
    );

    auto location_tool = createTool(
        "get_location",
        "Returns the vehicle's current location as coordinates and optionally a street address.",
        {
            {"address",   "Return a street address in addition to coordinates", "boolean", true},
            {"latitude",  "Latitude of the location",  "number", false},
            {"longitude", "Longitude of the location", "number", false},
        },
        [](const JsonObject& params) -> ToolResult {
            bool want_address = params.contains("address") && params["address"].get<bool>();
            JsonObject loc;
            loc["latitude"]  = 37.8052;
            loc["longitude"] = -122.432091;
            if (want_address) loc["address"] = "2 Marina Blvd, San Francisco, CA";

            std::string result = "Current location: (" +
                std::to_string(loc["latitude"].get<double>()) + ", " +
                std::to_string(loc["longitude"].get<double>()) + ")";
            if (want_address) result += " — " + loc["address"].get<std::string>();
            return ToolResult{true, result, {{"location", loc}}};
        }
    );

    auto ac_tool = createTool(
        "air_conditioner_control",
        "Adjusts the vehicle's climate control system.",
        {
            {"temperature", "Desired temperature in °F",         "integer", true},
            {"fan_speed",   "Fan speed 1–5",                     "integer", false},
            {"mode",        "Mode: cool, heat, or auto",         "string",  false},
        },
        [&vehicle](const JsonObject& params) -> ToolResult {
            int temp      = params["temperature"].get<int>();
            int fan       = params.contains("fan_speed") ? params["fan_speed"].get<int>() : 3;
            std::string m = params.contains("mode") ? params["mode"].get<std::string>() : "auto";

            std::string msg = "Climate set to " + std::to_string(temp) + "°F, fan " +
                              std::to_string(fan) + ", mode " + m + ".";
            JsonObject state; state["temperature"] = temp; state["fan_speed"] = fan; state["mode"] = m;
            { std::lock_guard<std::mutex> lk(vehicle.ac_mutex); vehicle.ac_json = state.dump(); }
            return ToolResult{true, msg, {{"ac_controls", msg}}};
        }
    );

    context->registerTool(navigation_tool);
    context->registerTool(location_tool);
    context->registerTool(ac_tool);

    // ── Agent ─────────────────────────────────────────────────────────────────
    VoiceAgent::Config cfg;
    cfg.agent_endpoint = "http://127.0.0.1:8080/agent";
    cfg.stt_endpoint   = "http://127.0.0.1:8888/";
    cfg.tts_endpoint   = "http://127.0.0.1:9999/";
    cfg.api_key        = config.get("EDGEAI_API_KEY");

    VoiceAgent agent(context, cfg);
    agent.init();

    // ── UI server ─────────────────────────────────────────────────────────────
    InCarUI ui;
    agent.cb_.onPartialResponse = [&ui](const std::string& chunk) { ui.onPartialResponse(chunk); };
    agent.cb_.onFinalResponse   = [&ui](const std::string& text)  { ui.onFinalResponse(text);   };
    ui.start(agent, context, cfg, vehicle);

    // ── Main loop ─────────────────────────────────────────────────────────────
    Logger::info("Press 'a' to listen, or type a message (or 'exit' to quit):");

    std::string input;
    while (true) {
        Logger::chunk("\n> ");
        std::getline(std::cin, input);
        if (input.empty()) continue;
        if (input == "exit" || input == "quit" || input == "q") break;
        if (input == "a") { agent.listen(); continue; }

        agent.runAsync(input, [](const JsonObject& update) {
            if (update.contains("part") && !update["part"].is_null())
                Logger::chunk(update["part"].get<std::string>());
            if (update.contains("answer"))
                Logger::chunk("\n");
        });
    }

    agent.stop();
    ui.stop();
    return EXIT_SUCCESS;
}