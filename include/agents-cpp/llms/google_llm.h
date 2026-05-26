/**
 * @file google_llm.h
 * @brief Google LLM Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/http_client.h>
#include <agents-cpp/llm_interface.h>

namespace agents {
namespace llms {

/**
 * @brief Implementation of LLMInterface for Google Gemini models
 */
class GoogleLLM : public LLMInterface {
public:
    /**
     * @brief Constructor
     * @param api_key The API key
     * @param model The model to use
     */
    GoogleLLM(const std::string& api_key = "", const std::string& model = "gemini-2.0-flash");

    /**
     * @brief Destructor
     */
    ~GoogleLLM() override = default;

    /**
     * @brief Get available models from Google
     * @return The available models
     */
    std::vector<std::string> getAvailableModels() override;

    /**
     * @brief Set the model to use
     * @param model The model to use
     */
    void setModel(const std::string& model) override;

    /**
     * @brief Get current model
     * @return The current model
     */
    std::string getModel() const override;

    /**
     * @brief Set API key
     * @param api_key The API key
     */
    void setApiKey(const std::string& api_key) override;

    /**
     * @brief Set API base URL (for self-hosted or proxied endpoints)
     * @param api_base The API base URL
     */
    void setApiBase(const std::string& api_base) override;

    /**
     * @brief Set options for API calls
     * @param options The options
     */
    void setOptions(const LLMOptions& options) override;

    /**
     * @brief Get current options
     * @return The current options
     */
    LLMOptions getOptions() const override;

    /**
     * @brief Generate completion from a prompt
     * @param prompt The prompt
     * @return The completion
     */
    LLMResponse chat(const std::string& prompt) override;

    /**
     * @brief Generate completion from a list of messages
     * @param messages The messages
     * @return The completion
     */
    LLMResponse chat(const std::vector<Message>& messages) override;

    /**
     * @brief Generate completion with available tools
     * @param messages The messages
     * @param tools The tools
     * @return The completion
     */
    LLMResponse chatWithTools(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    ) override;

    /**
     * @brief Stream results with callback
     * @param messages The messages
     * @param callback The callback
     */
    void streamChat(
        const std::vector<Message>& messages,
        std::function<void(const std::string&, bool)> callback
    ) override;

    /**
     * @brief Async completion chat
     * @param messages The messages
     * @param tools The tools
     * @return AsyncGenerator<std::string> The async generator with response
     */
    AsyncGenerator<std::string> streamChatAsync(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    ) override;

    /**
     * @brief Async completion chat with tools
     * @param messages The messages
     * @param tools The tools
     * @return AsyncGenerator<std::string> The async generator with response and tool calls
     */
    AsyncGenerator<std::pair<std::string, ToolCalls>> streamChatAsyncWithTools(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    ) override;

    /**
     * @brief Upload a local media file to the provider's file storage and
     *        return a canonical media envelope (e.g., with fileUri).
     * @param local_path Local filesystem path
     * @param mime The MIME type of the media file
     * @param binary Optional binary content of the media file
     * @return Optional envelope; std::nullopt if unsupported
     */
    std::optional<JsonObject> uploadMediaFile(const std::string& local_path, const std::string& mime, const std::string& binary = "") override;

private:
    std::string api_key_;
    std::string api_base_ = "https://generativelanguage.googleapis.com/v1";
    std::string model_;
    LLMOptions options_;
    HTTPClient http_client_;

    /**
     * @brief Convert Message list to Google API format
     * @param messages The messages
     * @param tools The tools
     * @return The Google API format
     */
    JsonObject formatMessages(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools = std::vector<std::shared_ptr<Tool>>());

    /**
     * @brief Convert Google API response to LLMResponse
     * @param response The response
     * @return The LLMResponse
     */
    LLMResponse parseGoogleResponse(const JsonObject& response);

    /**
     * @brief Make API call to Google
     * @param request_body The request body
     * @param stream Whether to stream the response
     * @return The response
     */
    JsonObject makeApiCall(const JsonObject& request_body, bool stream = false);

    /**
     * @brief Get the endpoint URL based on the model
     * @return The endpoint URL
     */
    std::string getEndpoint() const;

    /**
     * @brief Map media envelope to Google parts array
     * @param env The media envelope
     * @param out_parts The output parts array
     * @return True if the envelope was mapped, false otherwise
     */
    bool mapEnvelopeToGoogle(const JsonObject& env, JsonObject& out_parts);

    /**
     * @brief Start a resumable session
     * @param api_key The API key
     * @param content_length The content length
     * @param mime The MIME type
     * @param display_name The display name
     * @return The resumable session
     */
    std::string startResumableSession(const std::string& api_key, int64_t content_length, const std::string& mime, const std::string& display_name) const;

    /**
     * @brief Upload bytes to the resumable session
     * @param upload_url The upload URL
     * @param data The data to upload
     * @param content_length The content length
     * @return The response
     */
    JsonObject uploadBytesFinalize(const std::string& upload_url, const std::string& data, int64_t content_length) const;

    /**
     * @brief Wait for file to become ACTIVE after upload
     *
     * @param file_uri    The file URI
     * @param max_wait_ms Maximum wait time in milliseconds
     * @param sleep_ms    Sleep time between checks in milliseconds
     * @return true       if file is ACTIVE, false otherwise
     */
    bool waitForFileActive(const std::string& file_uri, int max_wait_ms, int sleep_ms) const;
};

} // namespace llms
} // namespace agents
