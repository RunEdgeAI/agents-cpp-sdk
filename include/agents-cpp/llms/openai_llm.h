/**
 * @file openai_llm.h
 * @brief OpenAI LLM Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/http_client.h>
#include <agents-cpp/llm_interface.h>

namespace agents {
namespace llms {

/**
 * @brief Implementation of LLMInterface for OpenAI models
 */
class OpenAILLM : public LLMInterface {
public:
    /**
     * @brief Constructor
     * @param api_key The API key
     * @param model The model to use
     */
    OpenAILLM(const std::string& api_key = "", const std::string& model = "gpt-4.1");

    /**
     * @brief Destructor
     */
    ~OpenAILLM() override = default;

    /**
     * @brief Get available models from OpenAI
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
private:
    std::string api_key_;
    std::string api_base_ = "https://api.openai.com/v1";
    std::string model_;
    LLMOptions options_;
    HTTPClient http_client_;

    /**
     * @brief Convert Message list to OpenAI API format
     * @param messages The messages
     * @return The OpenAI API format
     */
    JsonObject formatMessages(const std::vector<Message>& messages,
        bool stream = false,
        const std::vector<std::shared_ptr<Tool>>& tools = std::vector<std::shared_ptr<Tool>>());

    /**
     * @brief Convert Tool list to OpenAI API format
     * @param tools The tools
     * @return The OpenAI API format
     */
    JsonObject toolsToOpenAIFormat(const std::vector<std::shared_ptr<Tool>>& tools);

    /**
     * @brief Convert OpenAI API response to LLMResponse
     * @param response The response
     * @return The LLMResponse
     */
    LLMResponse parseOpenAIResponse(const JsonObject& response);

    /**
     * @brief Make API call to OpenAI
     * @param request_body The request body
     * @param stream Whether to stream the response
     * @param endpoint The endpoint to use
     * @return The response
     */
    JsonObject makeApiCall(const JsonObject& request_body, bool stream = false, const std::string& endpoint = "chat/completions");

    /**
     * @brief Map media envelope to OpenAI message content
     * @param env The media envelope
     * @param out_content The output content
     * @return True if the envelope was mapped, false otherwise
     */
    bool mapEnvelopeToOpenAI(const JsonObject& env, JsonObject& out_content);
};

} // namespace llms
} // namespace agents
