/**
 * @file anthropic_llm.h
 * @brief Anthropic LLM Definition
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
/**
 * @brief Large Language Models Namespace
 *
 */
namespace llms {

/**
 * @brief Implementation of LLMInterface for Anthropic Claude models
 */
class AnthropicLLM : public LLMInterface {
public:
    /**
     * @brief Constructor
     * @param api_key The API key
     * @param model The model to use
     */
    AnthropicLLM(const std::string& api_key = "", const std::string& model = "claude-3-5-sonnet-20240620");

    /**
     * @brief Destructor
     */
    ~AnthropicLLM() override = default;

    /**
     * @brief Get available models from Anthropic
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
     * @brief Stream chat with AsyncGenerator
     * @param messages The messages to generate completion from
     * @param tools The tools to use
     * @return The AsyncGenerator of response chunks
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
     * @brief Provider-optional: Upload a local media file to the provider's file storage and
     *        return a canonical media envelope (e.g., with fileUri). Default: not supported.
     * @param local_path Local filesystem path
     * @param mime The MIME type of the media file
     * @param binary Optional binary content of the media file
     * @return Optional envelope; std::nullopt if unsupported
     */
    std::optional<JsonObject> uploadMediaFile(const std::string& local_path, const std::string& mime, const std::string& binary) override;
private:
    std::string api_key_;
    std::string api_base_ = "https://api.anthropic.com";
    std::string model_;
    LLMOptions options_;
    HTTPClient http_client_;

    /**
     * @brief Convert Message list to Anthropic API format
     * @param messages The messages
     * @return The Anthropic API format
     */
    JsonObject formatMessages(const std::vector<Message>& messages,
        bool stream = false,
        const std::vector<std::shared_ptr<Tool>>& tools = std::vector<std::shared_ptr<Tool>>()
    );

    /**
     * @brief Convert Tool list to Anthropic API format
     * @param tools The tools
     * @return The Anthropic API format
     */
    JsonObject toolsToAnthropicFormat(const std::vector<std::shared_ptr<Tool>>& tools);

    /**
     * @brief Convert Anthropic API response to LLMResponse
     * @param response The response
     * @return The LLMResponse
     */
    LLMResponse parseAnthropicResponse(const JsonObject& response);

    /**
     * @brief Make API call to Anthropic
     * @param request_body The request body
     * @param stream Whether to stream the response
     * @return The response
     */
    JsonObject makeApiCall(const JsonObject& request_body, bool stream = false);

    /**
     * @brief Map media envelope to Anthropic message content blocks
     * @param env The media envelope
     * @param out_content_array The output content array
     * @return True if the envelope was mapped, false otherwise
     */
    bool mapEnvelopeToAnthropic(const JsonObject& env, JsonObject& out_content_array);

    /**
     * @brief Upload bytes as a file to Anthropic
     * @param name The file name
     * @param data The file data
     * @param mime The MIME type
     * @return The response JSON object
     */
    JsonObject uploadBytesFinalize(const std::string& name, const std::string& data, const std::string& mime) const;

    /**
     * @brief Wait for uploaded file to become active
     * @param file_uri The file URI
     * @param max_wait_ms Maximum wait time in milliseconds
     * @param sleep_ms Sleep interval in milliseconds
     * @return True if the file is active, false otherwise
     */
    bool waitForFileActive(const std::string& file_uri, int max_wait_ms, int sleep_ms) const;
};

} // namespace llms
} // namespace agents
