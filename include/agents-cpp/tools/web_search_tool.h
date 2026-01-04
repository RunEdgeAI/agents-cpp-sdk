/**
 * @file web_search_tool.h
 * @brief Web Search Tool Header - Simple and LLM-powered
 * @version 0.3
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 */
#ifndef AGENTS_CPP_TOOLS_WEB_SEARCH_TOOL_H
#define AGENTS_CPP_TOOLS_WEB_SEARCH_TOOL_H

#include <agents-cpp/tool.h>
#include <agents-cpp/llm_interface.h>
#include <memory>
#include <string>
#include <vector>

namespace agents {
namespace tools {

/**
 * @brief Structured search result item
 */
struct SearchResult {
    /// Title of the result
    std::string title;
    /// Snippet or summary of the result
    std::string snippet;
    /// URL of the result
    std::string url;
    /// Source name (e.g., website or publication)
    std::string source_name;
    /// Date of the result (if available)
    std::string date;
    /// Position in the search results
    int position;

    /**
     * @brief Convert SearchResult to JsonObject
     * @return JsonObject json representation of the SearchResult
     */
    JsonObject toJson() const {
        JsonObject obj;
        obj["title"] = title;
        obj["snippet"] = snippet;
        obj["url"] = url;
        obj["source_name"] = source_name;
        obj["position"] = position;
        if (!date.empty()) {
            obj["date"] = date;
        }
        return obj;
    }
};

/**
 * @brief Knowledge graph information
 */
struct KnowledgeGraphInfo {
    /// Title of the knowledge graph item
    std::string title;
    /// Description of the knowledge graph item
    std::string description;
    /// Type of the knowledge graph item
    std::string type;
    /// Additional attributes
    std::map<std::string, std::string> attributes;

    /**
     * @brief Check if the KnowledgeGraphInfo is empty
     * @return true if empty, false otherwise
     */
    bool isEmpty() const {
        return title.empty() && description.empty();
    }

    /**
     * @brief Convert KnowledgeGraphInfo to JsonObject
     * @return JsonObject json representation of the KnowledgeGraphInfo
     */
    JsonObject toJson() const {
        JsonObject obj;
        obj["title"] = title;
        obj["description"] = description;
        if (!type.empty()) {
            obj["type"] = type;
        }
        if (!attributes.empty()) {
            JsonObject attrs;
            for (const auto& [key, value] : attributes) {
                attrs[key] = value;
            }
            obj["attributes"] = attrs;
        }
        return obj;
    }
};

/**
 * @brief Web Search Tool with LLM-powered summarization
 *
 * This tool performs web searches and returns structured results.
 * The LLM is used to generate natural, contextual summaries of search results.
 */
class WebSearchTool : public Tool {
public:
    /**
     * @brief Construct a new Web Search Tool
     * @param llm Shared pointer to LLM interface for generating summaries
     */
    explicit WebSearchTool(std::shared_ptr<LLMInterface> llm);

    /**
     * @brief Execute the web search
     */
    ToolResult execute(const JsonObject& params) const override;

private:
    std::shared_ptr<LLMInterface> llm_;

    void setupParameters();

    // Core search methods
    ToolResult performWebSearch(const std::string& query) const;
    ToolResult performSerpApiSearch(const std::string& query) const;

    // Security validation
    bool isDangerousQuery(const std::string& query) const;
    bool isValidAgentSearchQuery(const std::string& query) const;

    // Result processing
    ToolResult processSerpApiResults(
        const std::string& query,
        const JsonObject& responseJson,
        int statusCode) const;

    // Polling for async results
    ToolResult pollForResults(
        const std::string& query,
        const JsonObject& initialResponse,
        const std::string& source) const;

    // Extract structured data
    std::vector<SearchResult> extractSearchResults(const JsonObject& responseJson) const;
    KnowledgeGraphInfo extractKnowledgeGraph(const JsonObject& responseJson) const;

    // Simple formatting for LLM consumption
    std::string formatResultsForLLM(
        const std::string& query,
        const std::vector<SearchResult>& results,
        const KnowledgeGraphInfo& kg) const;

    // LLM-powered summarization
    std::string generateLLMSummary(
        const std::string& query,
        const std::vector<SearchResult>& results,
        const KnowledgeGraphInfo& kg) const;

    // Helper methods
    std::string extractSourceName(const std::string& url) const;
};

} // namespace tools
} // namespace agents

#endif // AGENTS_CPP_TOOLS_WEB_SEARCH_TOOL_H