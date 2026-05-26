/**
 * @file wiki_tool.h
 * @brief Wikipedia Search Tool Header
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
 * @brief Wikipedia search tool that uses the MediaWiki API
 */
class WikipediaTool : public Tool {
public:
    /**
     * @brief Construct a new Wikipedia Tool object
     */
    WikipediaTool();

    /**
     * @brief Execute the Wikipedia Tool
     * @param params The parameters for the Wikipedia Tool
     * @return ToolResult The result of the Wikipedia Tool
     */
    ToolResult execute(const JsonObject& params) const override;

/*! @cond PRIVATE */
protected:
    /**
     * @brief Setup the parameters for the Wikipedia Tool
     */
    void setupParameters();

    /**
     * @brief Search Wikipedia
     * @param query The query to search for
     * @param limit The limit of results to return
     * @param lang The language to search in
     * @return ToolResult The result of the Wikipedia search
     */
    ToolResult searchWikipedia(const std::string& query, int limit, const std::string& lang) const;

    /**
     * @brief Fetch page details
     * @param page_ids The page IDs to fetch details for
     * @param lang The language to fetch details in
     * @return ToolResult The result of the page details fetch
     */
    ToolResult fetchPageDetails(const std::vector<std::string>& page_ids, const std::string& lang) const;

    /**
     * @brief Format the results
     * @param query The query that was searched for
     * @param lang The language that was searched in
     * @param search_results The results of the Wikipedia search
     * @param page_details The details of the Wikipedia pages
     * @return ToolResult The formatted results
     */
    ToolResult formatResults(const std::string& query, const std::string& lang,
                           const ToolResult& search_results, const ToolResult& page_details) const;
/*! @endcond */
};

} // namespace tools
} // namespace agents
