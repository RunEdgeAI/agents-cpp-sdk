/**
 * @file voice_agent.h
 * @brief Voice Agent Definition
 * @version 0.1
 * @date 2025-11-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/http_client.h>

namespace agents {

/**
 * @brief Voice agent that inherits from AutonomousAgent and adds voice capabilities
 */
class VoiceAgent : public AutonomousAgent {
public:
    /**
     * @brief Configuration for the VoicePipeline
     */
    struct Config {
        /// Endpoint URL for this agent
        std::string agent_endpoint;
        /// STT Endpoint URL
        std::string stt_endpoint;
        /// TTS Endpoint URL
        std::string tts_endpoint;
    };

    /**
     * @brief Callbacks for voice events
     */
    struct Callbacks {
        /**
         * @brief Called when transcription starts
         */
        std::function<void()> onTranscriptStart;

        /**
         * @brief Called with the partial transcript
         */
        std::function<void(const std::string& text)> onPartialTranscript;

        /**
         * @brief Called with the final transcript
         */
        std::function<void(const std::string& text)> onFinalTranscript;

        /**
         * @brief Called with the partial response from the agent
         */
        std::function<void(const std::string& text)> onPartialResponse;

        /**
         * @brief Called with the final response from the agent
         */
        std::function<void(const std::string& text)> onFinalResponse;

        /**
         * @brief Called when audio synthesis starts
         */
        std::function<void()> onSpeechStart;

        /**
         * @brief Called when audio synthesis ends
         */
        std::function<void()> onSpeechEnd;
    };

    /**
     * @brief Voice agent states
     */
    enum class State {
        /// INVALID state
        INVALID,
        /// IDLE state
        IDLE,
        /// Listening for voice input
        LISTENING,
        /// Processing voice input
        PROCESSING,
        /// SPEAKING state
        SPEAKING
    };

    /**
     * @brief Constructor
     * @param context The agent context
     * @param config The voice configuration
     */
    VoiceAgent(std::shared_ptr<Context> context, const Config& config);

    /**
     * @brief Destructor
     */
    ~VoiceAgent() override;

    /**
     * @brief Initialize the agent
     */
    void init() override;

    /**
     * @brief Listen for voice input
     */
    void listen();

    /**
     * @brief Stop the voice agent
     */
    void stop() override;

    /**
     * @brief Get the voice agent state
     * @return State
     */
    State getState();

    /**
     * @brief Set the voice agent state
     * @param state The new state
     */
    void setState(State state);

    /// Callbacks for voice agent events
    Callbacks cb_;
private:
    /**
     * @brief The agent state
     */
    State state_ = State::IDLE;
    /**
     * @brief Voice configuration
     */
    Config config_;
    /**
     * @brief This Agent's Endpoint Server
     */
    httplib::Server http_server_;
    /**
     * @brief Speech-to-text Endpoint Client
     */
    HTTPClient http_client_stt_;
    /**
     * @brief Text-to-speech Endpoint Client
     */
    HTTPClient http_client_;
    /**
     * @brief Thread for running the server
     */
    std::thread server_thread_;

    /**
     * @brief Handle STT events received from the STT service
     * @param eventJson The event JSON object
     */
    void handleEvent(JsonObject& eventJson);
    /**
     * @brief Send a request to the TTS service
     * @param api The TTS api
     * @param request The request body
     */
    void requestTTS(const std::string& api, const std::string& request);
    /**
     * @brief Send a request to the STT service
     * @param api The STT api
     * @param request The request body
     */
    void requestSTT(const std::string& api, const std::string& request);
};

/**
 * @brief Run a simple web UI for a VoiceAgent
 * @param media_dir The media directory containing the UI files
 * @param svr The HTTP server
 * @param agent The VoiceAgent
 */
static void runUI(const std::string& media_dir, httplib::Server& svr, VoiceAgent& agent) {
    // 1. SERVE THE UI
    svr.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        std::string html = Utils::loadHtmlFile(media_dir);
        res.set_content(html, "text/html");
    });

    // 2. THE TRIGGER API
    svr.Post("/api/trigger", [&agent](const httplib::Request&, httplib::Response& res) {
        Logger::info("[Backend] Agent triggered via Web UI!");
        agent.listen();
        res.set_content("OK", "text/plain");
    });

    // 3. THE TRANSCRIPT API
    svr.Get("/api/transcript", [&agent](const httplib::Request&, httplib::Response& res) {
        auto state = agent.getState();
        std::string state_str;

        switch (state) {
            case VoiceAgent::State::LISTENING: state_str = "1"; break;
            case VoiceAgent::State::PROCESSING: state_str = "2"; break;
            case VoiceAgent::State::SPEAKING: state_str = "3"; break;
            default: state_str = "0"; break;
        }

        res.set_header("X-Voice-Status", state_str);
        // Ensure this doesn't crash if memory is empty
        auto context = agent.getContext();
        if (context && context->getMemory()) {
            res.set_content(context->getMemory()->getConversationSummary(), "text/plain");
        } else {
            res.set_content("", "text/plain");
        }
    });

    // 4. THE STOP API
    svr.Post("/api/stop", [&agent](const httplib::Request&, httplib::Response& res) {
        // This stops audio playback and clears the LLM generation queue
        Logger::info("[Backend] Stop requested via Web UI!");
        agent.stop();
        res.set_content("OK", "text/plain");
    });

    Logger::info("UI server listening on http://localhost:9000");
    svr.listen("0.0.0.0", 9000);
}

} // namespace agents
