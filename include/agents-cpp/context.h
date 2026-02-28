/**
 * @file context.h
 * @brief Context Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/coroutine_utils.h>
#include <agents-cpp/llm_interface.h>
#include <agents-cpp/memory.h>
#include <agents-cpp/tool.h>
#include <agents-cpp/tools/tool_registry.h>
#include <agents-cpp/types.h>
#include <map>
#include <memory>
#include <vector>

/**
 * @brief Framework Namespace
 *
 */
namespace agents {

/**
 * @brief Context for an agent, containing tools, LLM, and memory
 */
class Context {
public:
    /**
     * @brief Default constructor for Context
     *
     */
    Context();

    /**
     * @brief Default destructor for Context
     */
    ~Context() = default;

    /**
     * @brief Set the LLM to use
     * @param llm The LLM to use
     */
    void setLLM(std::shared_ptr<LLMInterface> llm);

    /**
     * @brief Get the LLM
     * @return The LLM
     */
    std::shared_ptr<LLMInterface> getLLM() const;

    /**
     * @brief Set the system prompt
     * @param system_prompt The system prompt to use
     */
    void setSystemPrompt(const std::string& system_prompt);

    /**
     * @brief Get the system prompt
     * @return The system prompt
     */
    const std::string& getSystemPrompt() const;

    /**
     * @brief Register tools from a ToolRegistry
     * @param registry The ToolRegistry to register from
     */
    void registerToolRegistry(tools::ToolRegistry& registry);

    /**
     * @brief Register a tool
     * @param tool The tool to register
     */
    void registerTool(std::shared_ptr<Tool> tool);

    /**
     * @brief Get a tool by name
     * @param name The name of the tool to get
     * @return Pointer to tool
     */
    std::shared_ptr<Tool> getTool(const std::string& name) const;

    /**
     * @brief Get all tools
     * @return The tools
     */
    std::vector<std::shared_ptr<Tool>> getTools() const;

    /**
     * @brief Execute a tool by name using coroutines
     * @param name The name of the tool to execute
     * @param params The parameters to pass to the tool
     * @param tool_call_id Optional tool call ID
     * @return The result of the tool execution
     */
    Task<ToolResult> executeTool(const std::string& name, const JsonObject& params, const std::string& tool_call_id = "");

    /**
     * @brief Get the memory
     * @return The memory
     */
    std::shared_ptr<Memory> getMemory() const;

    /**
     * @brief Add a message to the conversation history
     * @param message The message to add
     */
    void addMessage(const Message& message);

    /**
     * @brief Get all messages in the conversation history
     * @return The messages
     */
    std::vector<Message> getMessages() const;

    /**
     * @brief Multimodal chat completion with the current context
     * @param user_message The user message to send
     * @param uris_or_data Optional URIs or data to use
     * @return The LLM response
     */
    Task<LLMResponse> chat(const std::string user_message, const std::vector<std::string> uris_or_data = {});

    /**
     * @brief Multimodal chat completion with tools
     * @param user_message The user message to send
     * @param uris_or_data Optional URIs or data to use
     * @return The LLM response
     */
    Task<LLMResponse> chatWithTools(const std::string user_message, const std::vector<std::string> uris_or_data = {});

    /**
     * @brief Multimodal streaming chat (accepts one or more media URIs or data strings)
     * @param user_message The user message to send
     * @param uris_or_data Optional URIs or data to use
     * @return The LLM response
     */
    AsyncGenerator<std::string> streamChat(const std::string user_message, const std::vector<std::string> uris_or_data = {});

    /**
     * @brief  Multimodal streaming chat with tools
     * @param user_message The user message to send
     * @param uris_or_data Optional URIs or data to use
     * @return The LLM response
     */
    AsyncGenerator<std::string> streamChatWithTools(const std::string user_message, const std::vector<std::string> uris_or_data = {});

private:
    /**
     * @brief The LLM to use
     */
    std::shared_ptr<LLMInterface> llm_;

    /**
     * @brief The memory to use
     */
    std::shared_ptr<Memory> memory_;

    /**
     * @brief The tools to use
     */
    std::map<std::string, std::shared_ptr<Tool>> tools_;

    /**
     * @brief The system prompt to use
     */
    std::string system_prompt_;

    /**
     * @brief Build message from multimodal parts
     * @param prompt The prompt to send to the LLM
     * @param uris_or_data The URIs or data to use
     * @return The message
     */
    Message buildMultimodalParts(const std::string& prompt, const std::vector<std::string>& uris_or_data);
};

} // namespace agents
