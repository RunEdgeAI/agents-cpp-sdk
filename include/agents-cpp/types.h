/**
 * @file types.h
 * @brief Types Definitions
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <any>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace agents {

/**
 * @brief JSON object type
 * @note This is a JSON object type. It contains the JSON object.
 */
using JsonObject = nlohmann::json;

/**
 * @brief JSON array type
 * @note This is a JSON array type. It contains the JSON array.
 */
using JsonArray = nlohmann::json::array_t;

/**
 * @brief Parameter type for tools and LLM calls
 * @note This is a parameter type for tools and LLM calls. It contains the name, description, type, required, and default value.
 */
struct Parameter {
    /**
     * @brief The name of the parameter
     */
    std::string name;
    /**
     * @brief The description of the parameter
     */
    std::string description;
    /**
     * @brief The type of the parameter
     */
    std::string type;
    /**
     * @brief Whether the parameter is required
     */
    bool required;
    /**
     * @brief The default value of the parameter
     */
    std::optional<JsonObject> default_value = std::nullopt;
    /**
     * @brief Element schema for `type: "array"` parameters
     * @note Required by some providers (e.g. Gemini). Defaults to
     * `{"type": "string"}` when ingested from a source schema that omits it.
     */
    std::optional<JsonObject> items = std::nullopt;
    /**
     * @brief Nested property schema for `type: "object"` parameters
     * @note Captured from the source JSON schema's `properties` field.
     */
    std::optional<JsonObject> properties = std::nullopt;
    /**
     * @brief Allowed enum values for the parameter
     * @note Captured from the source JSON schema's `enum` field.
     */
    std::optional<JsonObject> enum_values = std::nullopt;
};

/**
 * @brief Parameter map type
 * @note This is a parameter map type. It contains the string and parameter.
 */
using ParameterMap = std::map<std::string, Parameter>;

/**
 * @brief A single tool call emitted by an LLM.
 * @note `id` is the call identifier returned by the LLM. It must be echoed
 * back in the matching tool-result message's `tool_call_id` so the model can
 * thread results to calls. Some providers (e.g. Gemini) don't return an id;
 * adapters synthesize one (e.g. "call_<n>") in that case.
 */
struct ToolCall {
    /**
     * @brief tool id.
     */
    std::string id;
    /**
     * @brief tool name.
     */
    std::string name;
    /**
     * @brief tool arguments.
     */
    JsonObject arguments;
    /**
     * @brief Opaque provider-specific token that must round-trip with the
     * tool call when echoed back to the LLM in subsequent turns.
     * @note Currently only populated by Gemini (`thoughtSignature`). Other
     * adapters leave it empty and ignore it on serialization.
     */
    std::string signature;

    ToolCall() = default;
    /**
     * @brief Construct a new Tool Call object
     * @param id_        tool id.
     * @param name_      tool name.
     * @param arguments_ tool args.
     */
    ToolCall(std::string id_, std::string name_, JsonObject arguments_)
        : id(std::move(id_)), name(std::move(name_)), arguments(std::move(arguments_)) {}
    /**
     * @brief Construct a new Tool Call object
     * @param id_        tool id.
     * @param name_      tool name.
     * @param arguments_ tool args.
     * @param signature_ tool signature.
     */
    ToolCall(std::string id_, std::string name_, JsonObject arguments_, std::string signature_)
        : id(std::move(id_)), name(std::move(name_)),
          arguments(std::move(arguments_)), signature(std::move(signature_)) {}
};

/**
 * @brief Tool calls type — a vector of ToolCall entries.
 */
using ToolCalls = std::vector<ToolCall>;

/**
 * @brief Response from an LLM
 * @note This is the response from an LLM call. It contains the content of the response,
 * the tool calls that were made, and the usage metrics for the call.
 */
struct LLMResponse {
    /**
     * @brief The content of the response
     */
    std::string content;
    /**
     * @brief The tool call id of the response
     */
    std::string tool_call_id;
    /**
     * @brief The tool calls that were made
     */
    ToolCalls tool_calls;
    /**
     * @brief The usage metrics for the call
     */
    std::map<std::string, double> usage_metrics;
};

/**
 * @brief Message in a conversation
 * @note This is a message in a conversation. It contains the role of the message,
 * the content of the message, the name of the message, the tool call id, and the tool calls.
 */
struct Message {
    /**
     * @brief The role of the message
     */
    enum class Role {
        /**
         * @brief System role message
         */
        SYSTEM,
        /**
         * @brief User role message
         */
        USER,
        /**
         * @brief Assistant role message
         */
        ASSISTANT,
        /**
         * @brief Tool role message
         */
        TOOL
    };

    /**
     * @brief The role of the message
     */
    Role role;
    /**
     * @brief The content of the message
     */
    std::string content;
    /**
     * @brief The name of the message
     */
    std::optional<std::string> name = std::nullopt;
    /**
     * @brief The tool call id of the message
     */
    std::optional<std::string> tool_call_id = std::nullopt;
    /**
     * @brief The tool calls that were made
     */
    ToolCalls tool_calls = {};
};

/**
 * @brief Trust level for the context execution environment
 */
enum class TrustLevel {
    FULL,       /**< Full access to all tools and capabilities */
    SANDBOXED,  /**< Sandboxed access, tools might be restricted or simulated */
    READONLY    /**< Read-only access, state mutating tools are blocked */
};

/**
 * @brief Memory types
 * @note This is a type of memory. It contains the type of memory, the name of the memory,
 * and the content of the memory.
 */
enum class MemoryType {
    /**
     * @brief Short term memory
     */
    SHORT_TERM,
    /**
     * @brief Long term memory
     */
    LONG_TERM,
    /**
     * @brief Working memory
     */
    WORKING
};

/**
 * @brief MCP server config
 */
struct mcpConfig {
    /**
     * @brief Name of the server
     */
    std::string serverName;
    /**
     * @brief url of the server
     */
    std::string url;
    /**
     * @brief Type of server: sse | stdio
     */
    std::string type;
    /**
     * @brief Command to run
     */
    std::string command;
    /**
     * @brief args to pass to command
     */
    std::vector<std::string> args;
    /**
     * @brief env variables
     */
    nlohmann::ordered_json env_vars = nlohmann::ordered_json::object();
};

} // namespace agents
