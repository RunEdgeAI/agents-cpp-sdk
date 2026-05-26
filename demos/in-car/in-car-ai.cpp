/**
 * @example in-car-ai.cpp
 * @brief In-Car AI Demo
 * @version 0.4
 * @date 2026-05-02
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */
#include "ui.hpp"
#include "nav_tools.hpp"

#include <agents-cpp/config_loader.h>
#include <agents-cpp/tools/tool_registry.h>

#include <iostream>

using namespace agents;

int main() {
    Logger::init(Logger::Level::INFO);

    // ── Config + LLM ──────────────────────────────────────────────────────────
    auto& config = ConfigLoader::getInstance();
    std::shared_ptr<LLMInterface> llm;
    try {
        llm = createLLM(config.get("PROVIDER"), config.get("API_KEY"), config.get("MODEL"));
        auto effort = config.get("THINK");
        if (!effort.empty()) {
            auto options = llm->getOptions();
            options.reasoning_effort = effort;
            llm->setOptions(options);
        }
    } catch (const std::exception& e) {
        Logger::error("Error creating LLM: {}", e.what());
        return EXIT_FAILURE;
    }

    // ── Context ───────────────────────────────────────────────────────────────
    auto context = std::make_shared<Context>(llm);

    // Persona, behavior rules, MCP servers, and skills loaded from markdown files.
    // demos/in-car/.agents/ is scanned first, cwd/.agents/, then root .agents/.
    context->loadWorkspaceConfig("demos/in-car/.agents");

    // ── Tools ─────────────────────────────────────────────────────────────────
    tools::ToolRegistry registry = tools::ToolRegistry::global();
    registry.registerStandardTools(llm);
    context->registerToolRegistry(registry);

    // Shared vehicle state — written by tools below, read by the UI server
    VehicleState vehicle;
    incar_nav::registerNavTools(*context, vehicle);

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

            // query a free IP-based geolocation service.
            JsonObject loc;
            try {
                auto resp = HTTPClient::get("https://ipapi.co/json/", {}, {{"User-Agent", "agents-cpp/in-car"}}, 5000);
                if (!resp.error && resp.status_code == 200) {
                    auto body = JsonObject::parse(resp.text);
                    loc["latitude"]  = body.value("latitude", 0.0);
                    loc["longitude"] = body.value("longitude", 0.0);
                    if (want_address) {
                        std::string addr;
                        if (body.contains("city"))    addr += body.value("city", "");
                        if (body.contains("region"))  addr += (addr.empty() ? "" : ", ") + body.value("region", "");
                        if (body.contains("country_code")) addr += (addr.empty() ? "" : ", ") + body.value("country_code", "");
                        if (!addr.empty()) loc["address"] = addr;
                    }
                }
            } catch (...) { /* fall through to error below */ }

            if (!loc.contains("latitude") || !loc.contains("longitude")) {
                return ToolResult{false,
                    "Unable to determine current location (geolocation lookup failed).",
                    JsonObject()};
            }

            std::string result = "Current location: (" +
                std::to_string(loc["latitude"].get<double>()) + ", " +
                std::to_string(loc["longitude"].get<double>()) + ")";
            if (loc.contains("address")) result += " — " + loc["address"].get<std::string>();
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
        if (!std::getline(std::cin, input)) break;
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
