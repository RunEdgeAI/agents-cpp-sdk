/**
 * @file types.h
 * @brief Types Definitions
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
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
};

/**
 * @brief Parameter map type
 * @note This is a parameter map type. It contains the string and parameter.
 */
using ParameterMap = std::map<std::string, Parameter>;

/**
 * @brief Tool calls type
 * @note This is a tool calls type. It contains the tool name and parameters.
 */
using ToolCalls = std::vector<std::pair<std::string, JsonObject>>;

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
    std::vector<std::pair<std::string, JsonObject>> tool_calls = {};
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
