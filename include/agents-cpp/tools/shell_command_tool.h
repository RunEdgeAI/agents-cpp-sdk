/**
 * @file shell_command_tool.h
 * @brief Shell Command Tool Header
 * @version 0.1
 * @date 2025-08-24
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/tool.h>

namespace agents {
namespace tools {

/**
 * @brief Shell command tool that provides secure command execution capabilities
 */
class ShellCommandTool : public Tool {
public:
    /**
     * @brief Construct a new Shell Command Tool object
     */
    ShellCommandTool();

    /**
     * @brief Execute the Shell Command Tool
     * @param params The parameters for the Shell Command Tool
     * @return ToolResult The result of the Shell Command Tool
     */
    ToolResult execute(const JsonObject& params) const override;

/*! @cond PRIVATE */
protected:
    /**
     * @brief Setup the parameters for the Shell Command Tool
     */
    void setupParameters();

    /**
     * @brief Validate the command
     * @param command The command to validate
     * @return true if the command is valid, false otherwise
     */
    bool validateCommand(const std::string& command) const;

    /**
     * @brief Check if the command is dangerous
     * @param command The command to check
     * @return true if the command is dangerous, false otherwise
     */
    bool isDangerousCommand(const std::string& command) const;

    /**
     * @brief Execute the command
     * @param command The command to execute
     * @return ToolResult The result of the command execution
     */
    ToolResult executeCommand(const std::string& command) const;
/*! @endcond */
};

} // namespace tools
} // namespace agents
