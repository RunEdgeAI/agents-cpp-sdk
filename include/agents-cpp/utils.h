/**
 * @file utils.h
 * @brief Utility functions for the project
 * @version 0.1
 * @date 2025-10-25
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */

#pragma once

#include <agents-cpp/types.h>
#include <agents-cpp/logger.h>

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

namespace agents {

/**
 * @brief Utility class providing static helper functions
 */
class Utils {
public:
    /**
     * @brief Change a key in a JSON object
     *
     * @param object The JSON object
     * @param old_key The old key name
     * @param new_key The new key name
     */
    static void changeKey(JsonObject& object, const std::string& old_key, const std::string& new_key) {
        // get iterator to old key; TODO: error handling if key is not present
        auto it = object.find(old_key);
        if (it == object.end()) {
            throw std::runtime_error("Key not found: " + old_key);
        }
        // create null value for new key and swap value from old key
        std::swap(object[new_key], it.value());
        // delete value at old key (cheap, because the value is null after swap)
        object.erase(it);
    }

    /**
     * @brief Convert a string to lowercase
     *
     * @param s The input string
     * @return The lowercase string
     */
    static inline std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return result;
    }

    /**
     * @brief Convert a string to uppercase
     *
     * @param s The input string
     * @return std::string
     */
    static inline std::string toUpper(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
        return result;
    }


    /**
     * @brief Split a string by newlines
     * @param text The text to split
     * @return The lines
     */
    static std::vector<std::string> splitByNewline(const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream iss(text);
        std::string line;

        // Read lines from the stringstream until the end, delimited by '\n'
        if (!text.empty())
        {
            while (std::getline(iss, line, '\n')) {
                lines.push_back(line);
            }
        }
        return lines;
    }

    /**
     * @brief String trim helper
     * @param s the input string
     */
    static void trim(std::string& s) {
        auto not_space = [](unsigned char c){ return !std::isspace(c); };

        // erase the the spaces at the back first
        // so we don't have to do extra work
        s.erase(
            std::ranges::find_if(s | std::views::reverse, not_space).base(),
            s.end());

        // erase the spaces at the front
        s.erase(
            s.begin(),
            std::ranges::find_if(s, not_space));
    }


    /**
     * @brief Extract the path (including leading '/') from a full URL.
     *
     * Example: `https://example.com/api/search?q=x` -> `/api/search?q=x`
     *
     * @param url Full URL
     * @return std::string Path and query portion
     */
    static std::string getBaseUrl(const std::string& url) {
        size_t proto_end = url.find("://");
        if (proto_end == std::string::npos) return url;
        size_t path_start = url.find("/", proto_end + 3);
        if (path_start == std::string::npos) return url;
        return url.substr(0, path_start);
    }

    /**
     * @brief Extract the path (including leading '/') from a full URL.
     *
     * Example: `https://example.com/api/search?q=x` -> `/api/search?q=x`
     *
     * @param url Full URL
     * @return std::string Path and query portion
     */
    static std::string getPathFromUrl(const std::string& url) {
        size_t proto_end = url.find("://");
        if (proto_end == std::string::npos) return "/";
        size_t path_start = url.find("/", proto_end + 3);
        if (path_start == std::string::npos) return "/";
        return url.substr(path_start);
    }

    /**
     * @brief Get the hostname From Url object
     * @details (e.g., "127.0.0.1") stripping "http://" and ":8080"
     * @param url The URL string
     * @return std::string The hostname portion of the URL
     */
    static std::string getHostFromUrl(const std::string& url) {
        size_t proto_end = url.find("://");
        // Start searching after "://" if it exists, otherwise start at 0
        size_t search_start = (proto_end != std::string::npos) ? proto_end + 3 : 0;

        // Find the end of the host (first occurrence of ':' for port or '/' for path)
        size_t host_end = url.find_first_of(":/?", search_start);

        return url.substr(search_start, host_end - search_start);
    }

    /**
     * @brief Get the port from a URL string
     * @details Returns the port as an integer (defaulting to 80 or 443 if not found)
     * @param url The URL string
     * @return int The port number
     */
    static int getPortFromUrl(const std::string& url) {
        size_t proto_end = url.find("://");
        int port = (proto_end != std::string::npos && url.substr(0, proto_end) == "https") ? 443 : 80;

        size_t search_start = (proto_end != std::string::npos) ? proto_end + 3 : 0;
        size_t path_start = url.find_first_of("/?", search_start); // Stop search at path/query

        // Look for colon in the authority section
        // Substring is just the part between "://" and "/"
        std::string authority = (path_start == std::string::npos)
                                ? url.substr(search_start)
                                : url.substr(search_start, path_start - search_start);

        size_t colon_pos = authority.find(':');
        if (colon_pos != std::string::npos) {
            // Parse the numbers after the colon
            port = std::stoi(authority.substr(colon_pos + 1));
        }

        return port;
    }

    /**
     * @brief Load an HTML file from disk
     * @param filename The path to the HTML file
     * @return The file contents as a string
     */
    static std::string loadHtmlFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::error("Could not open file: {}", filename);
            return "<h1>Error: Could not load file</h1>";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    /**
     * @brief Strip HTML tags from a string, producing plain text
     *
     * Removes script/style/nav/header/footer/noscript blocks entirely,
     * strips remaining tags, decodes common HTML entities, and collapses whitespace.
     *
     * @param html The HTML string to strip
     * @param max_length Maximum output length (0 = unlimited). Truncates at word boundary.
     * @return Plain text extracted from the HTML
     */
    static std::string stripHtmlTags(const std::string& html, size_t max_length = 0) {
        if (html.empty()) {
            return "";
        }

        // Step 1: Remove block-level elements that typically contain non-content
        std::string working = html;
        std::vector<std::string> blockTags = {
            "script", "style", "nav", "header", "footer", "noscript"
        };

        for (const auto& tag : blockTags) {
            std::regex blockRegex(
                "<" + tag + "[^>]*>[\\s\\S]*?</" + tag + "\\s*>",
                std::regex::icase);
            working = std::regex_replace(working, blockRegex, " ");
        }

        // Step 2: Replace <br>, <br/>, <br />, <p>, <div>, <li>, <tr> with newlines
        std::regex lineBreakRegex("<(?:br|/p|/div|/li|/tr|/h[1-6])[^>]*>", std::regex::icase);
        working = std::regex_replace(working, lineBreakRegex, "\n");

        // Step 3: Strip remaining HTML tags using state machine
        std::string text;
        text.reserve(working.size());
        bool inTag = false;

        for (size_t i = 0; i < working.size(); ++i) {
            char c = working[i];
            if (c == '<') {
                inTag = true;
            } else if (c == '>') {
                inTag = false;
                text += ' ';  // Replace tag with space to prevent word joining
            } else if (!inTag) {
                text += c;
            }
        }

        // Step 4: Decode common HTML entities
        auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos) {
                str.replace(pos, from.length(), to);
                pos += to.length();
            }
        };

        replaceAll(text, "&amp;", "&");
        replaceAll(text, "&lt;", "<");
        replaceAll(text, "&gt;", ">");
        replaceAll(text, "&quot;", "\"");
        replaceAll(text, "&#39;", "'");
        replaceAll(text, "&apos;", "'");
        replaceAll(text, "&nbsp;", " ");
        replaceAll(text, "&#160;", " ");

        // Decode numeric entities (&#NNN;)
        std::regex numericEntityRegex("&#(\\d+);");
        std::string decoded;
        std::sregex_iterator it(text.begin(), text.end(), numericEntityRegex);
        std::sregex_iterator end;
        size_t lastPos = 0;

        for (; it != end; ++it) {
            decoded += text.substr(lastPos, it->position() - lastPos);
            int codePoint = std::stoi((*it)[1].str());
            if (codePoint >= 32 && codePoint < 127) {
                decoded += static_cast<char>(codePoint);
            } else {
                decoded += ' ';
            }
            lastPos = it->position() + it->length();
        }
        decoded += text.substr(lastPos);
        text = std::move(decoded);

        // Step 5: Collapse whitespace
        // Replace runs of spaces/tabs with single space
        std::string collapsed;
        collapsed.reserve(text.size());
        bool lastWasSpace = false;
        bool lastWasNewline = false;

        for (char c : text) {
            if (c == '\n' || c == '\r') {
                if (!lastWasNewline) {
                    collapsed += '\n';
                    lastWasNewline = true;
                }
                lastWasSpace = false;
            } else if (c == ' ' || c == '\t') {
                if (!lastWasSpace && !lastWasNewline) {
                    collapsed += ' ';
                    lastWasSpace = true;
                }
            } else {
                collapsed += c;
                lastWasSpace = false;
                lastWasNewline = false;
            }
        }
        text = std::move(collapsed);

        // Trim leading/trailing whitespace
        Utils::trim(text);

        // Step 6: Truncate at word boundary if max_length specified
        if (max_length > 0 && text.size() > max_length) {
            size_t cutPos = max_length;
            // Find the last space before the cut point
            while (cutPos > 0 && text[cutPos] != ' ' && text[cutPos] != '\n') {
                --cutPos;
            }
            if (cutPos == 0) {
                cutPos = max_length;  // No word boundary found, hard cut
            }
            text = text.substr(0, cutPos) + "...";
        }

        return text;
    }

    /**
     * @brief Extract the text content of the first occurrence of a given HTML tag
     *
     * @param html The HTML string to search
     * @param tag The tag name to find (e.g., "title")
     * @return The text content within the first matching tag, or empty string if not found
     */
    static std::string extractHtmlTagContent(const std::string& html, const std::string& tag) {
        if (html.empty() || tag.empty()) {
            return "";
        }

        // Find opening tag (case-insensitive manual search)
        std::string lowerHtml = Utils::toLower(html);
        std::string lowerTag = Utils::toLower(tag);

        std::string openPattern = "<" + lowerTag;
        size_t openPos = lowerHtml.find(openPattern);
        if (openPos == std::string::npos) {
            return "";
        }

        // Find the end of the opening tag
        size_t tagEnd = lowerHtml.find('>', openPos);
        if (tagEnd == std::string::npos) {
            return "";
        }

        size_t contentStart = tagEnd + 1;

        // Find closing tag
        std::string closePattern = "</" + lowerTag;
        size_t closePos = lowerHtml.find(closePattern, contentStart);
        if (closePos == std::string::npos) {
            return "";
        }

        // Extract content using original (non-lowered) html to preserve casing
        std::string content = html.substr(contentStart, closePos - contentStart);

        // Strip any nested tags from the content
        std::string plainText;
        bool inTag = false;
        for (char c : content) {
            if (c == '<') {
                inTag = true;
            } else if (c == '>') {
                inTag = false;
            } else if (!inTag) {
                plainText += c;
            }
        }

        Utils::trim(plainText);
        return plainText;
    }
};

} // namespace agents
