/**
 * @file datetime_tool.h
 * @brief Datetime Tool Header
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
 * @brief Tool that returns the current date, time, day of week, and timezone
 */
class DatetimeTool : public Tool {
public:
    /**
     * @brief Construct a new DatetimeTool object
     */
    DatetimeTool();

    /**
     * @brief Execute the DatetimeTool
     * @param params The parameters for the DatetimeTool (none required)
     * @return ToolResult The result of the DatetimeTool
     */
    ToolResult execute(const JsonObject& params) const override;
};

} // namespace tools
} // namespace agents
