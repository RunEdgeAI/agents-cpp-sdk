/**
 * @file weather_tool.h
 * @brief Weather Tool Header
 * @version 0.1
 * @date 2025-10-31
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/tool.h>

namespace agents {
namespace tools {

/**
 * @brief Tool that fetches current weather conditions for any location via wttr.in
 */
class WeatherTool : public Tool {
public:
    /**
     * @brief Construct a new WeatherTool object
     */
    WeatherTool();

    /**
     * @brief Execute the WeatherTool
     * @param params The parameters for the WeatherTool
     * @return ToolResult The result of the WeatherTool
     */
    ToolResult execute(const JsonObject& params) const override;
};

} // namespace tools
} // namespace agents
