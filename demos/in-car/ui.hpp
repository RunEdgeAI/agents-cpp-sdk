/**
 * @file ui.hpp
 * @brief In-Car AI HTTP UI server — header-only
 */
#pragma once

#include <agents-cpp/agents/voice_agent.h>
#include <agents-cpp/http_client.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/utils.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace agents;

// Shared vehicle state written by tool callbacks in main, read by HTTP handlers here.
struct VehicleState {
    std::mutex  nav_mutex;
    std::string nav_json;

    std::mutex  ac_mutex;
    std::string ac_json;
};

class InCarUI {
public:
    // Wired to VoiceAgent streaming callbacks
    void onPartialResponse(const std::string& chunk) {
        std::lock_guard<std::mutex> lk(partial_mutex_);
        current_partial_ += chunk;
    }

    void onFinalResponse(const std::string&) {
        std::lock_guard<std::mutex> lk(partial_mutex_);
        current_partial_.clear();
    }

    // Start the HTTP server in a background thread (non-blocking)
    void start(VoiceAgent& agent,
               std::shared_ptr<Context> ctx,
               const VoiceAgent::Config& cfg,
               VehicleState& state,
               int port = 9000) {

        svr_.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(Utils::loadHtmlFile("sample_media/ui/in-car.html"), "text/html");
        });

        svr_.Post("/api/trigger", [&agent](const httplib::Request&, httplib::Response& res) {
            Logger::info("[Backend] Agent triggered via Web UI!");
            agent.listen();
            res.set_content("OK", "text/plain");
        });

        svr_.Get("/api/transcript", [&agent, ctx, this](const httplib::Request&, httplib::Response& res) {
            const char* state_str = "0";
            switch (agent.getState()) {
                case VoiceAgent::State::LISTENING:  state_str = "1"; break;
                case VoiceAgent::State::PROCESSING: state_str = "2"; break;
                case VoiceAgent::State::SPEAKING:   state_str = "3"; break;
                default: break;
            }
            std::string summary;
            if (ctx && ctx->getMemory())
                summary = ctx->getMemory()->getConversationSummary();
            {
                std::lock_guard<std::mutex> lk(partial_mutex_);
                if (!current_partial_.empty())
                    summary += "Assistant: " + current_partial_ + "\n\n";
            }
            res.set_header("X-Voice-Status", state_str);
            res.set_content(summary, "text/plain");
        });

        svr_.Get("/api/navigation", [&state](const httplib::Request&, httplib::Response& res) {
            std::lock_guard<std::mutex> lk(state.nav_mutex);
            if (state.nav_json.empty()) { res.status = 204; return; }
            res.set_content(state.nav_json, "application/json");
        });

        svr_.Get("/api/music", [ctx](const httplib::Request&, httplib::Response& res) {
            auto tool = ctx->getTool("itunes_current_song");
            if (!tool) { res.status = 204; return; }
            try {
                auto r = tool->execute({});
                if (r.success && !r.content.empty()) {
                    std::string lower = r.content;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    bool silent = lower.find("nothing")     != std::string::npos ||
                                  lower.find("not playing") != std::string::npos ||
                                  lower.find("no track")    != std::string::npos ||
                                  lower.find("stopped")     != std::string::npos ||
                                  lower.find("error")       != std::string::npos ||
                                  lower.find("can't get")   != std::string::npos;
                    if (silent) res.status = 204;
                    else        res.set_content(r.content, "text/plain");
                } else { res.status = 204; }
            } catch (...) { res.status = 204; }
        });

        svr_.Get("/api/climate", [&state](const httplib::Request&, httplib::Response& res) {
            std::lock_guard<std::mutex> lk(state.ac_mutex);
            if (state.ac_json.empty()) { res.status = 204; return; }
            res.set_content(state.ac_json, "application/json");
        });

        svr_.Post("/api/stop", [&agent](const httplib::Request&, httplib::Response& res) {
            Logger::info("[Backend] Stop requested via Web UI!");
            agent.stop();
            res.set_content("OK", "text/plain");
        });

        svr_.Get("/api/voices", [&cfg](const httplib::Request&, httplib::Response& res) {
            try {
                std::map<std::string, std::string> headers;
                auto r = HTTPClient::get(cfg.tts_endpoint + "voices", headers);
                if (r.status_code == 200 && !r.text.empty())
                    res.set_content(r.text, "application/json");
                else
                    res.status = r.status_code > 0 ? r.status_code : 502;
            } catch (...) { res.status = 502; }
        });

        svr_.Post("/api/loadVoice", [&cfg](const httplib::Request& req, httplib::Response& res) {
            try {
                std::map<std::string, std::string> headers = {{"Content-Type", "text/plain"}};
                if (!cfg.api_key.empty())
                    headers["Authorization"] = "Bearer " + cfg.api_key;
                auto r = HTTPClient::post(cfg.tts_endpoint + "loadVoice", headers, req.body);
                res.status = r.status_code;
                res.set_content(r.text, "text/plain");
            } catch (...) { res.status = 502; }
        });

        svr_.Post("/api/clear", [&agent](const httplib::Request&, httplib::Response& res) {
            Logger::info("[Backend] Clear context requested via Web UI!");
            auto c = agent.getContext();
            if (c && c->getMemory()) c->getMemory()->clear();
            res.set_content("OK", "text/plain");
        });

        thread_ = std::thread([this, port]() {
            Logger::info("In-Car UI server listening on http://127.0.0.1:{}", port);
            svr_.listen("0.0.0.0", port);
        });
    }

    void stop() {
        svr_.stop();
        if (thread_.joinable()) thread_.join();
    }

private:
    httplib::Server svr_;
    std::thread     thread_;
    std::mutex      partial_mutex_;
    std::string     current_partial_;
};