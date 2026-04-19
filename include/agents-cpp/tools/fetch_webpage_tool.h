/**
 * @file fetch_webpage_tool.h
 * @brief Fetch Webpage Tool Header
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
 * @brief Tool that fetches and returns the plain-text content of any web page
 */
class FetchWebpageTool : public Tool {
public:
    /**
     * @brief Construct a new FetchWebpageTool object
     */
    FetchWebpageTool();

    /**
     * @brief Execute the FetchWebpageTool
     * @param params The parameters for the FetchWebpageTool
     * @return ToolResult The result of the FetchWebpageTool
     */
    ToolResult execute(const JsonObject& params) const override;
};

} // namespace tools
} // namespace agents
