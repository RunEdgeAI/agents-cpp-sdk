/**
 * @file ollama_llm.h
 * @brief Ollama LLM Definition
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
 * @brief Implementation of LLMInterface for Ollama models
 */
class OllamaLLM : public LLMInterface {
public:
    /**
     * @brief Constructor
     * @param api_key The API key
     * @param model The model to use
     */
    OllamaLLM(const std::string& api_key, const std::string& model = "llama3");
    /**
     * @brief Destructor
     */
    ~OllamaLLM() override = default;

    /**
     * @brief Get available models from Ollama
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
     * @brief Set API key (not used for Ollama, but implemented for interface compliance)
     * @param api_key The API key
     */
    void setApiKey(const std::string& api_key) override;

    /**
     * @brief Set API base URL for Ollama server
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
     * @brief Stream results asynchronously
     * @param messages The messages
     * @param tools The tools
     * @return AsyncGenerator of response chunks
     */
    AsyncGenerator<std::string> streamChatAsync(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    ) override;

    /**
     * @brief Stream chat with tools asynchronously
     * @param messages The messages
     * @param tools The tools
     * @return AsyncGenerator of response chunks and possible toolcalls
     */
    AsyncGenerator<std::pair<std::string, ToolCalls>> streamChatAsyncWithTools(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    ) override;

private:
    std::string api_key_;
    std::string api_base_ = "http://localhost:11434/api";
    std::string model_;
    LLMOptions options_;
    HTTPClient http_client_;

    /**
     * @brief Convert Message list to Ollama API format
     * @param messages The messages
     * @param stream Whether to stream the response
     * @param tools The tools (optional)
     * @return The Ollama request body
     */
    JsonObject formatMessages(
        const std::vector<Message>& messages,
        bool stream = false,
        const std::vector<std::shared_ptr<Tool>>& tools = std::vector<std::shared_ptr<Tool>>());

    /**
     * @brief Convert Ollama API response to LLMResponse
     * @param response The response
     * @return The LLMResponse
     */
    LLMResponse parseOllamaResponse(const JsonObject& response);

    /**
     * @brief Make API call to Ollama
     * @param request_body The request body
     * @param stream Whether to stream the response
     * @return The response
     */
    JsonObject makeApiCall(const JsonObject& request_body, bool stream = false);

    /**
     * @brief Format message history for models without chat format support
     * @param messages The messages
     * @return The formatted messages
     */
    std::string formatMessagesAsPrompt(const std::vector<Message>& messages);

    /**
     * @brief Check if the model supports chat format
     * @return True if the model supports chat format
     */
    bool modelSupportsChatFormat() const;

    /**
     * @brief Check if the model supports tool calls
     * @return True if the model supports tool calls
     */
    bool modelSupportsToolCalls() const;

    /**
     * @brief Apply media envelope to Ollama message JSON (content/images)
     * @param env The media envelope
     * @param inout_msg The output message JSON
     * @return True if the envelope was mapped, false otherwise
     */
    bool applyEnvelopeToOllamaMessage(const JsonObject& env, JsonObject& inout_msg);

    /**
     * @brief Helper for stripping data url prefix
     * @param value input string
     * @return std::string trimmed output string
     */
    static inline std::string strip_data_url_prefix(const std::string& value) {
        if (value.rfind("data:", 0) == 0) {
            size_t comma_pos = value.find(',');
            if (comma_pos != std::string::npos && comma_pos + 1 < value.size()) {
                return value.substr(comma_pos + 1);
            }
            return std::string();
        }
        return value;
    }
};

} // namespace llms
} // namespace agents
