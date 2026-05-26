/**
 * @file http_client.h
 * @brief Thin HTTP Client Wrapper
 * @version 0.2
 * @date 2025-10-11
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace agents {

/**
 * @brief Thin HTTP Client wrapper for internal use.
 *
 * Usage contract:
 * - Callers receive an `HTTPClient::Response` and should inspect
 *   `response.error` and `response.status_code` to decide success.
 * - For streaming responses provide a WriteCallback to `post()` to receive
 *   incremental body chunks.
 */
class HTTPClient {
public:
    /**
     * @brief http headers
     */
    using Headers = std::map<std::string, std::string>;
    /**
     * @brief http params
     */
    using Params = std::multimap<std::string, std::string>;

    /**
     * @brief Callback type for streaming response body data.
     *
     * The callback receives a string_view containing the next chunk of
     * response body data. It should return true to continue receiving
     * data or false to abort the transfer.
     */
    using WriteCallback = std::function<bool(const std::string_view&)>;

    /**
     * @brief Multipart form data part.
     */
    struct MultipartFormData {
        std::string name;           ///< part name
        std::string content;        ///< part content
        std::string filename;       ///< part filename
        std::string content_type;   ///< part content type
    };

    /**
     * @brief Normalized response returned by the wrapper functions.
     *
     * - `status_code` contains the HTTP status or -1 on network/connect failure.
     * - `text` contains the full response body when no streaming callback was
     *    provided. For streaming calls the body will be empty.
     * - `error` is true when a transport or library error occurred.
     * - `error_message` contains an explanatory message for transport errors.
     */
    struct Response {
        int status_code = -1;      /**< HTTP status code or -1 on error */
        std::string text;          /**< Response body (when available) */
        bool error = false;        /**< True when there was a transport error */
        std::string error_message; /**< Transport error or HTTP reason */
        Headers headers;           /**< Response headers */
    };

    /**
     * @brief Construct a persistent HTTPClient.
     *
     * Establishes the base connection context. The TCP connection is maintained
     * alive where possible (Keep-Alive).
     */
    explicit HTTPClient(const std::string& url);
    ~HTTPClient();

    // Disable copying to prevent socket race conditions.
    HTTPClient(const HTTPClient&) = delete;
    HTTPClient& operator=(const HTTPClient&) = delete;
    /**
     * @brief Safe move constructor
     */
    HTTPClient(HTTPClient&&) noexcept;
    /**
     * @brief Safe move constructor with equals operator
     * @return HTTPClient& Http client object
     */
    HTTPClient& operator=(HTTPClient&&) noexcept;

    /**
     * @brief Instance GET (persistent).
     * @param url The URL to GET.
     * @param headers The headers to include.
     * @param timeout_ms The timeout in milliseconds.
     */
    Response Get(const std::string& url,
                 const Headers& headers = {},
                 int timeout_ms = 30000);

    /**
     * @brief Instance POST (persistent).
     * @param url The URL to POST to.
     * @param headers The headers to include.
     * @param body The body to include.
     * @param timeout_ms The timeout in milliseconds.
     * @param write_cb Optional streaming write callback.
     * @param multipart Optional multipart form data.
     */
    Response Post(const std::string& url,
                  const Headers& headers,
                  const std::string& body,
                  int timeout_ms = 30000,
                  WriteCallback write_cb = nullptr,
                  const std::vector<MultipartFormData>& multipart = {});

    /**
     * @brief Static POST (transient).
     * @param url The URL to POST to.
     * @param headers The headers to include.
     * @param body The body to include.
     * @param timeout_ms The timeout in milliseconds.
     * @param write_cb Optional streaming write callback.
     * @param multipart Optional multipart form data.
     * @return Response The HTTP response.
     */
    static Response post(const std::string& url,
                         const Headers& headers,
                         const std::string& body,
                         int timeout_ms = 30000,
                         WriteCallback write_cb = nullptr,
                         const std::vector<MultipartFormData>& multipart = {});

    /**
     * @brief Static GET (transient).
     * @param url The URL to GET.
     * @param headers The headers to include.
     * @param timeout_ms The timeout in milliseconds.
     * @return Response The HTTP response.
     */
    static Response get(const std::string& url,
                        const Headers& headers,
                        int timeout_ms = 30000);

    /**
     * @brief Static GET with query parameters (transient).
     * @param url The URL to GET.
     * @param params The query parameters.
     * @param headers The headers to include.
     * @param timeout_ms The timeout in milliseconds.
     * @return Response The HTTP response.
     */
    static Response get(const std::string& url,
                        const Params& params,
                        const Headers& headers,
                        int timeout_ms = 30000);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace agents
