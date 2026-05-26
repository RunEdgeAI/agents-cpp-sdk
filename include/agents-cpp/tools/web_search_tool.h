/**
 * @file web_search_tool.h
 * @brief Web Search Tool Header - Simple and LLM-powered
 * @version 0.4
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
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
 * @brief Direct answer extracted from SerpAPI response (answer boxes, featured snippets, etc.)
 */
struct DirectAnswer {
    /// Type of direct answer: "answer_box", "featured_snippet", "sports_result", "rich_snippet"
    std::string type;
    /// The direct answer text
    std::string answer;
    /// Title associated with the answer
    std::string title;
    /// Extended snippet text
    std::string snippet;
    /// Source URL for the answer
    std::string source_url;
    /// Highlighted words from the answer
    std::vector<std::string> highlighted_words;

    /**
     * @brief Check if the DirectAnswer is empty
     * @return true if no meaningful content, false otherwise
     */
    bool isEmpty() const {
        return answer.empty() && snippet.empty() && title.empty();
    }

    /**
     * @brief Convert DirectAnswer to JsonObject
     * @return JsonObject json representation
     */
    JsonObject toJson() const {
        JsonObject obj;
        obj["type"] = type;
        if (!answer.empty()) obj["answer"] = answer;
        if (!title.empty()) obj["title"] = title;
        if (!snippet.empty()) obj["snippet"] = snippet;
        if (!source_url.empty()) obj["source_url"] = source_url;
        if (!highlighted_words.empty()) {
            JsonObject words = JsonObject::array();
            for (const auto& w : highlighted_words) {
                words.push_back(w);
            }
            obj["highlighted_words"] = words;
        }
        return obj;
    }
};

/**
 * @brief Content fetched from a search result page for deep analysis
 */
struct FetchedPageContent {
    /// URL that was fetched
    std::string url;
    /// Page title extracted from HTML
    std::string title;
    /// Extracted/summarized text content
    std::string content;
    /// LLM-generated summary (if available)
    std::string summary;
    /// Error message if fetch failed
    std::string error;
    /// Original HTML length before processing
    size_t original_length = 0;
    /// Whether the fetch was successful
    bool success = true;

    /**
     * @brief Convert FetchedPageContent to JsonObject
     * @return JsonObject json representation
     */
    JsonObject toJson() const {
        JsonObject obj;
        obj["url"] = url;
        obj["title"] = title;
        obj["content"] = content;
        if (!summary.empty()) obj["summary"] = summary;
        if (!error.empty()) obj["error"] = error;
        obj["original_length"] = original_length;
        obj["success"] = success;
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
    ToolResult performWebSearch(const std::string& query,
                                bool deep_fetch = false,
                                int deep_fetch_count = 2) const;
    ToolResult performSerpApiSearch(const std::string& query,
                                    bool deep_fetch = false,
                                    int deep_fetch_count = 2) const;

    // Security validation
    bool isDangerousQuery(const std::string& query) const;
    bool isValidAgentSearchQuery(const std::string& query) const;

    // Result processing
    ToolResult processSerpApiResults(
        const std::string& query,
        const JsonObject& responseJson,
        int statusCode,
        bool deep_fetch = false,
        int deep_fetch_count = 2) const;

    // Polling for async results
    ToolResult pollForResults(
        const std::string& query,
        const JsonObject& initialResponse,
        const std::string& source,
        bool deep_fetch = false,
        int deep_fetch_count = 2) const;

    // Extract structured data
    std::vector<SearchResult> extractSearchResults(const JsonObject& responseJson) const;
    KnowledgeGraphInfo extractKnowledgeGraph(const JsonObject& responseJson) const;
    DirectAnswer extractDirectAnswer(const JsonObject& responseJson) const;

    // Deep fetch methods
    std::vector<FetchedPageContent> fetchPageContents(
        const std::vector<SearchResult>& results,
        int count) const;
    FetchedPageContent fetchSinglePage(const std::string& url) const;
    std::string summarizePageContent(
        const std::string& query,
        const std::string& url,
        const std::string& content) const;

    // Simple formatting for LLM consumption
    std::string formatResultsForLLM(
        const std::string& query,
        const std::vector<SearchResult>& results,
        const KnowledgeGraphInfo& kg,
        const DirectAnswer& directAnswer = {},
        const std::vector<FetchedPageContent>& fetchedPages = {}) const;

    // Helper methods
    std::string extractSourceName(const std::string& url) const;
};

} // namespace tools
} // namespace agents

#endif // AGENTS_CPP_TOOLS_WEB_SEARCH_TOOL_H
