/**
 * @file voice_agent.h
 * @brief Voice Agent Definition
 * @version 0.1
 * @date 2025-11-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/http_client.h>

#include <atomic>
#include <memory>
#include <thread>

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
        /// API key for authenticating with STT/TTS services
        std::string api_key;
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
     * @brief Inject a user message into the agent reasoning loop, bypassing STT.
     * @param text User-provided text (typed)
     * @param speak If true, stream the response through TTS as well
     */
    void submitText(const std::string& text, bool speak = true);

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

    /**
     * @brief Getter for STT connected status
     *
     * @return true     is connected
     * @return false    not connected
     */
    bool isSTTConnected() const;

    /**
     * @brief Getter for TTS connected status
     *
     * @return true     is connected
     * @return false    not connected
     */
    bool isTTSConnected() const;

    /// Callbacks for voice agent events
    Callbacks cb_;
private:
    struct ServerState;
    using ServerHandle = std::unique_ptr<ServerState>;

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
    ServerHandle server_;
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
     * @brief Tracks last known reachability of TTS endpoint. Used to log only on transitions.
     */
    std::atomic<bool> tts_connected_{true};
    /**
     * @brief Tracks last known reachability of STT endpoint. Used to log only on transitions.
     */
    std::atomic<bool> stt_connected_{true};

    /**
     * @brief Handle STT events received from the STT service
     * @param eventJson The event JSON object
     */
    void handleEvent(JsonObject& eventJson);
    /**
     * @brief Send a request to the TTS service
     * @param api The TTS api
     * @param request The request body
     * @return true if the request succeeded, false otherwise
     */
    bool requestTTS(const std::string& api, const std::string& request);
    /**
     * @brief Send a request to the STT service
     * @param api The STT api
     * @param request The request body
     * @return true if the request succeeded, false otherwise
     */
    bool requestSTT(const std::string& api, const std::string& request);
    /**
     * @brief Probe the STT and TTS /health endpoints and log a warning if either is unreachable.
     */
    void checkServiceHealth();
};

} // namespace agents
