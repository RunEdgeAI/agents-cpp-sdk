/**
 * @file calculator_tool.h
 * @brief Calculator Tool Header
 * @version 0.1
 * @date 2025-10-31
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/tool.h>

namespace agents {
namespace tools {

/**
 * @brief Tool that evaluates mathematical expressions safely using Python
 */
class CalculatorTool : public Tool {
public:
    /**
     * @brief Construct a new CalculatorTool object
     */
    CalculatorTool();

    /**
     * @brief Execute the CalculatorTool
     * @param params The parameters for the CalculatorTool
     * @return ToolResult The result of the CalculatorTool
     */
    ToolResult execute(const JsonObject& params) const override;
};

} // namespace tools
} // namespace agents
