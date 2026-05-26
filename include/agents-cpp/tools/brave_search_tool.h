/**
 * @file brave_search_tool.h
 * @brief Brave Search API Tool
 * @version 0.1
 * @date 2025-04-11
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 * Requires WEBSEARCH_API_KEY set to a Brave Search API key.
 * Free tier: 2,000 queries/month — https://api.search.brave.com
 */
#pragma once

#include <agents-cpp/llm_interface.h>
#include <agents-cpp/tool.h>

#include <memory>
#include <string>

namespace agents {
namespace tools {

/**
 * @brief Web search tool backed by the Brave Search API.
 *
 * Significantly faster than SerpAPI — direct index, single HTTP round-trip,
 * no intermediate proxy. Set WEBSEARCH_API_KEY to your Brave API key.
 */
class BraveSearchTool : public Tool {
public:
    explicit BraveSearchTool();

    ToolResult execute(const JsonObject& params) const override;

private:
    ToolResult search(const std::string& query) const;
    std::string formatResponse(const std::string& query, const JsonObject& resp) const;

    // deep_fetch support (commented out — revisit if needed)
    // std::shared_ptr<LLMInterface> llm_;
    // ToolResult search(const std::string& query, bool deep_fetch, int deep_fetch_count) const;
    // std::string fetchAndSummarizePage(const std::string& url, const std::string& query) const;
};

} // namespace tools
} // namespace agents
