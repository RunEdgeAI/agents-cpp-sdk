/**
 * @file llm_interface.h
 * @brief LLM Interface Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/types.h>
#include <agents-cpp/tool.h>
#include <agents-cpp/coroutine_utils.h>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace agents {

// Recoverable per-turn LLM failure (e.g. the model produced malformed
// tool-call JSON the provider couldn't parse). Today only Ollama raises it.
class LLMRetryableTurnError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/**
 * @brief Options for LLM API calls
 */
struct LLMOptions {
    /**
     * @brief The temperature of the LLM
     */
    double temperature = 0.7;
    /**
     * @brief The maximum number of tokens
     */
    int max_tokens = 1024;
    /**
     * @brief The top p of the LLM
     */
    double top_p = 1.0;
    /**
     * @brief The presence penalty of the LLM
     */
    double presence_penalty = 0.0;
    /**
     * @brief The frequency penalty of the LLM
     */
    double frequency_penalty = 0.0;
    /**
     * @brief The timeout in milliseconds
     */
    int timeout_ms = 30000; // 30 seconds
    /**
     * @brief The stop sequences of the LLM
     */
    std::vector<std::string> stop_sequences;
    /**
     * @brief Response schema for structured output (JSON schema)
     */
    std::optional<JsonObject> response_schema;
    /**
     * @brief Response MIME type for structured output
     * @note Use "application/json" for JSON output, "text/x.enum" for enum output
     */
    std::optional<std::string> response_mime_type;
    /**
     * @brief Reasoning effort for thinking-capable models.
     *
     * Accepted values: "low", "medium", "high". Mapped per provider:
     *   - Ollama (gpt-oss): sets `think` to the level string (Reasoning: low|medium|high in template).
     *   - OpenAI (o1/o3/o4-mini/gpt-5): sets `reasoning_effort` API field.
     *   - Anthropic (Claude 4.x with extended thinking): sets `thinking.budget_tokens`
     *     using a level→budget mapping (low=1024, medium=4096, high=16384).
     *   - Google (Gemini): no equivalent yet; ignored.
     *
     * If unset (nullopt): Ollama defaults to `think: true` (model default level + channel split);
     * other providers don't send any reasoning param.
     */
    std::optional<std::string> reasoning_effort;
};

/**
 * @brief Interface for language model providers (OpenAI, Anthropic, Google, Ollama)
 */
class LLMInterface {
public:
    /**
     * @brief Destructor
     */
    virtual ~LLMInterface() = default;

    /**
     * @brief Get available models from this provider
     * @return The available models
     */
    virtual std::vector<std::string> getAvailableModels() = 0;

    /**
     * @brief Set the model to use
     * @param model The model to use
     */
    virtual void setModel(const std::string& model) = 0;

    /**
     * @brief Get current model
     * @return The current model
     */
    virtual std::string getModel() const = 0;

    /**
     * @brief Set API key
     * @param api_key The API key to use
     */
    virtual void setApiKey(const std::string& api_key) = 0;

    /**
     * @brief Set API base URL (for self-hosted or proxied endpoints)
     * @param api_base The API base URL to use
     */
    virtual void setApiBase(const std::string& api_base) = 0;

    /**
     * @brief Set options for API calls
     * @param options The options to use
     */
    virtual void setOptions(const LLMOptions& options) = 0;

    /**
     * @brief Get current options
     * @return The current options
     */
    virtual LLMOptions getOptions() const = 0;

    /**
     * @brief Generate completion from a prompt
     * @param prompt The prompt
     * @return The completion
     */
    virtual LLMResponse chat(const std::string& prompt) = 0;

    /**
     * @brief Generate completion from a list of messages
     * @param messages The messages to generate completion from
     * @return The LLM response
     */
    virtual LLMResponse chat(const std::vector<Message>& messages) = 0;

    /**
     * @brief Generate completion with available tools
     * @param messages The messages to generate completion from
     * @param tools The tools to use
     * @return The LLM response
     */
    virtual LLMResponse chatWithTools(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    ) = 0;

    /**
     * @brief Stream results with callback
     * @param messages The messages to generate completion from
     * @param callback The callback to use
     */
    virtual void streamChat(
        const std::vector<Message>& messages,
        std::function<void(const std::string&, bool)> callback
    ) = 0;

    /**
     * @brief Async chat from a list of messages
     * @param messages The messages to generate completion from
     * @return The LLM response
     */
    virtual Task<LLMResponse> chatAsync(const std::vector<Message>& messages);

    /**
     * @brief Async chat with tools
     * @param messages The messages to generate completion from
     * @param tools The tools to use
     * @return The LLM response
     */
    virtual Task<LLMResponse> chatWithToolsAsync(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    );

    /**
     * @brief Stream chat with AsyncGenerator
     * @param messages The messages to generate completion from
     * @param tools The tools to use
     * @return The AsyncGenerator of response chunks
     */
    virtual AsyncGenerator<std::string> streamChatAsync(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    );

    /**
     * @brief Stream chat with tools using AsyncGenerator
     * @param messages The messages to generate completion from
     * @param tools The tools to use
     * @return The AsyncGenerator of response chunks and tool calls
     */
    virtual AsyncGenerator<std::pair<std::string, ToolCalls>> streamChatAsyncWithTools(
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<Tool>>& tools
    );

    /**
     * @brief Provider-optional: Upload a local media file to the provider's file storage and
     *        return a canonical media envelope (e.g., with fileUri). Default: not supported.
     * @param local_path Local filesystem path
     * @param mime The MIME type of the media file
     * @param binary Optional binary content of the media file
     * @return Optional envelope; std::nullopt if unsupported
     */
    virtual std::optional<JsonObject> uploadMediaFile(const std::string& local_path, const std::string& mime, const std::string& binary = "");
};

/**
 * @brief Factory function to create a specific LLM provider
 *
 * @param provider One of: "anthropic", "openai", "google", "ollama"
 * @param api_key API key for the provider
 * @param model Model to use (provider-specific)
 * @return The LLMInterface
 */
std::shared_ptr<LLMInterface> createLLM(
    const std::string& provider,
    const std::string& api_key,
    const std::string& model = ""
);

} // namespace agents
