/**
 * @file utils.h
 * @brief Utility functions for the project
 * @version 0.1
 * @date 2025-10-25
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 */

#pragma once

#include <agents-cpp/types.h>

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
};

} // namespace agents
