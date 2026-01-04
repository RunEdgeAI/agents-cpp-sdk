/**
 * @file tool.h
 * @brief Tool Definitions
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/types.h>
#include <functional>

namespace agents {

/**
 * @brief Result of a tool execution
 * @note This is the result of a tool execution. It contains the success of the execution,
 * the content of the result, and the data of the result.
 */
struct ToolResult {
    /**
     * @brief Whether the tool execution was successful
     */
    bool success;
    /**
     * @brief The content of the tool result
     */
    std::string content;
    /**
     * @brief The data of the tool result
     */
    JsonObject data;
};

/**
 * @brief A ToolResult that represents a failure.
 */
struct ToolError : ToolResult {
    /**
     * @brief Construct a new ToolError object
     * @param error The error data
     */
    ToolError(const JsonObject& error) : ToolResult(false, {}, error) {}
};

/**
 * @brief Callback type for tool execution
 * @note This is the callback type for tool execution. It is a function that takes a json object and returns a tool result.
 */
using ToolCallback = std::function<ToolResult(const JsonObject&)>;

/**
 * @brief Interface for tools that an agent can use
 *
 * Tools are capabilities that the LLM can use to interact with the outside world.
 * Each tool has a name, description, set of parameters, and execution logic.
 */
class Tool {
public:
    /**
     * @brief Constructor
     * @param name The name of the tool
     * @param description The description of the tool
     */
    Tool(const std::string& name, const std::string& description);

    /**
     * @brief Construct a new Tool object
     *
     * @param name The name of the tool
     * @param description The description of the tool
     * @param parameters The json parameters of the tool
     */
    Tool(const std::string& name, const std::string& description, const JsonObject& parameters);

    /**
     * @brief Destructor
     */
    virtual ~Tool() = default;

    /**
     * @brief Get the name of the tool
     * @return The name of the tool
     */
    const std::string& getName() const;

    /**
     * @brief Get the description of the tool
     * @return The description of the tool
     */
    const std::string& getDescription() const;

    /**
     * @brief Get the parameters of the tool
     * @return The parameters of the tool
     */
    const ParameterMap& getParameters() const;

    /**
     * @brief Get the schema of the tool
     * @return The schema of the tool
     */
    const JsonObject& getSchema() const;

    /**
     * @brief Add a parameter to the tool
     * @param param The parameter to add
     */
    void addParameter(const Parameter& param);

    /**
     * @brief Add tool parameters from json definition
     * @param params The parameters to add
     */
    void addParametersFromJson(const JsonObject& params);

    /**
     * @brief Set the execution callback
     * @param callback The callback to set
     */
    void setCallback(ToolCallback callback);

    /**
     * @brief Execute the tool with the given parameters
     * @param params The parameters to execute the tool with
     * @return The result of the tool execution
     */
    virtual ToolResult execute(const JsonObject& params) const;

    /**
     * @brief Validate parameters against schema
     * @param params The parameters to validate
     * @return true of valid, otherwise false
     */
    bool validateParameters(const JsonObject& params) const;

/*! @cond PRIVATE */
protected:
    /**
     * @brief The name of the tool
     */
    std::string name_;
    /**
     * @brief The description of the tool
     */
    std::string description_;
    /**
     * @brief The parameters of the tool
     */
    ParameterMap parameters_;
    /**
     * @brief The callback of the tool
     */
    ToolCallback callback_;
    /**
     * @brief The schema of the tool
     */
    JsonObject schema_;

    /**
     * @brief Update schema when parameters change
     */
    void updateSchema();
/*! @endcond */
};

/**
 * @brief Create a custom tool with a name, description, parameters, and callback
 * @param name The name of the tool
 * @param description The description of the tool
 * @param parameters The parameters of the tool
 * @param callback The callback for the tool
 * @return A shared pointer to the tool
 */
std::shared_ptr<Tool> createTool(
    const std::string& name,
    const std::string& description,
    const std::vector<Parameter>& parameters,
    ToolCallback callback
);

} // namespace agents
