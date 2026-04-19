/**
 * @file python_tool.h
 * @brief Python Execution Tool Header
 * @version 0.1
 * @date 2025-08-24
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/tool.h>

namespace agents {
namespace tools {

/**
 * @brief Executes Python code via subprocess and returns captured stdout/stderr.
 *
 * Requires python3 to be available in PATH. No embedded interpreter — each
 * execution spawns a fresh python3 process, keeping the agent process isolated
 * from user code.
 */
class PythonTool : public Tool {
public:
    PythonTool();
    ToolResult execute(const JsonObject& params) const override;
};

} // namespace tools
} // namespace agents
