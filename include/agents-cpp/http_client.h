/**
 * @file http_client.h
 * @brief Thin HTTP Client Wrapper
 * @version 0.2
 * @date 2025-10-11
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 */
#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <functional>
#include <httplib.h>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <agents-cpp/utils.h>

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
     * @brief Callback type for streaming response body data.
     *
     * The callback receives a string_view containing the next chunk of
     * response body data. It should return true to continue receiving
     * data or false to abort the transfer.
     */
    using WriteCallback = std::function<bool(const std::string_view&)>;

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
        std::string error_message; /**< True when there was a transport error */
        httplib::Headers headers;  /**< Response headers */
    };

    /**
     * @brief Construct a Persistent HTTPClient
     * * Establishes the base connection context. The TCP connection is
     * maintained alive where possible (Keep-Alive).
     */
    explicit HTTPClient(const std::string& url) {
        // Initialize persistent client
        instance_client_ = std::make_unique<httplib::Client>(Utils::getBaseUrl(url));

        // Default persistent settings
        instance_client_->set_keep_alive(true);
        instance_client_->set_tcp_nodelay(true);
        instance_client_->set_connection_timeout(5); // 5s connect timeout
    }

    // Disable copying to prevent socket race conditions
    HTTPClient(const HTTPClient&) = delete;
    HTTPClient& operator=(const HTTPClient&) = delete;

    /**
     * @brief Instance GET (Persistent)
     * @param url The URL to GET
     * @param headers The headers to include
     * @param timeout_ms The timeout in milliseconds
     */
    Response Get(const std::string& url,
                 const std::map<std::string, std::string>& headers = {},
                 int timeout_ms = 30000) {
        std::scoped_lock lock(client_mutex_);

        // Update timeouts for this specific request
        instance_client_->set_read_timeout(timeout_ms / 1000);
        instance_client_->set_write_timeout(timeout_ms / 1000);

        // Convert headers
        httplib::Headers header_map;
        for (const auto& [key, value] : headers) {
            header_map.emplace(key, value);
        }

        Response result;
        try {
            auto res = instance_client_->Get(Utils::getPathFromUrl(url), header_map);
            processResult(res, result);
        } catch (const std::exception& e) {
            result.error = true;
            result.error_message = e.what();
        }
        return result;
    }

    /**
     * @brief Instance POST (Persistent)
     * @param url The URL to POST to
     * @param headers The headers to include
     * @param body The body to include
     * @param timeout_ms The timeout in milliseconds
     * @param write_cb Optional streaming write callback
     * @param multipart Optional multipart form data
     */
    Response Post(const std::string& url,
                  const std::map<std::string, std::string>& headers,
                  const std::string& body,
                  int timeout_ms = 30000,
                  WriteCallback write_cb = nullptr,
                  const std::vector<httplib::MultipartFormData>& multipart = {}) {
        std::scoped_lock lock(client_mutex_);

        // Reuse the internal logic, passing our persistent client
        return performRequest(instance_client_.get(), "POST", url, headers, body, timeout_ms, write_cb, multipart);
    }

    // =========================================================================
    // STATIC METHODS (Transient - Legacy Support)
    // =========================================================================
    /**
     * @brief Static POST (Transient)
     * @param url        The URL to POST to
     * @param headers    The headers to include
     * @param body       The body to include
     * @param timeout_ms The timeout in milliseconds
     * @param write_cb   Optional streaming write callback
     * @param multipart  Optional multipart form data
     * @return Response  The HTTP response
     */
    static Response post(const std::string& url,
                        const std::map<std::string, std::string>& headers,
                        const std::string& body,
                        int timeout_ms = 30000,
                        WriteCallback write_cb = nullptr,
                        const std::vector<httplib::MultipartFormData>& multipart = {}) {
        // Create transient client
        auto client = std::make_unique<httplib::Client>(Utils::getBaseUrl(url));
        client->set_connection_timeout(2);

        // Delegate to shared logic
        return performRequest(client.get(), "POST", url, headers, body, timeout_ms, write_cb, multipart);
    }

    /**
     * @brief Static GET (Transient)
     * @param url The URL to GET
     * @param headers The headers to include
     * @param timeout_ms The timeout in milliseconds
     * @return Response The HTTP response
     */
    static Response get(const std::string& url,
                       const std::map<std::string, std::string>& headers,
                       int timeout_ms = 30000) {
        Response result;
        try {
            httplib::Client cli(Utils::getBaseUrl(url));
            cli.set_connection_timeout(timeout_ms / 1000);
            cli.set_read_timeout(timeout_ms / 1000);

            httplib::Headers header_map;
            for (const auto& [key, value] : headers) header_map.emplace(key, value);

            auto res = cli.Get(Utils::getPathFromUrl(url), header_map);
            processResult(res, result);
        } catch (const std::exception& e) {
            result.error = true;
            result.error_message = e.what();
        }
        return result;
    }

    // Overload for query params
    /**
     * @brief Static GET with query parameters (Transient)
     * @param url        The URL to GET
     * @param params     The query parameters
     * @param headers    The headers to include
     * @param timeout_ms The timeout in milliseconds
     * @return Response  The HTTP response
     */
    static Response get(const std::string& url,
                       const httplib::Params& params,
                       const std::map<std::string, std::string>& headers,
                       int timeout_ms = 30000) {
        Response result;
        try {
            httplib::Client cli(Utils::getBaseUrl(url));
            cli.set_connection_timeout(timeout_ms / 1000);
            cli.set_read_timeout(timeout_ms / 1000);

            httplib::Headers header_map;
            for (const auto& [key, value] : headers) header_map.emplace(key, value);

            auto res = cli.Get(Utils::getPathFromUrl(url), params, header_map);
            processResult(res, result);
        } catch (const std::exception& e) {
            result.error = true;
            result.error_message = e.what();
        }
        return result;
    }

private:
    // Persistent state
    std::unique_ptr<httplib::Client> instance_client_;
    std::mutex client_mutex_;

    // Shared helper to populate Response object from httplib::Result
    static void processResult(const httplib::Result& res, Response& result) {
        if (res) {
            result.status_code = res->status;
            result.headers = res->headers;
            result.text = res->body;
            result.error = false;
            result.error_message = res->reason; // Add reason even on success
        } else {
            result.error = true;
            result.error_message = to_string(res.error());
            result.status_code = -1;
        }
    }

    // Centralized Request Logic (Used by both Static and Instance methods)
    static Response performRequest(httplib::Client* client,
                                   const std::string& method,
                                   const std::string& url,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& body,
                                   int timeout_ms,
                                   WriteCallback write_cb,
                                   const std::vector<httplib::MultipartFormData>& multipart) {
        Response result;
        try {
            // Apply request-specific timeouts
            client->set_read_timeout(timeout_ms / 1000);
            client->set_write_timeout(timeout_ms / 1000);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            // Assuming default verification for now, or add to method sig if needed
            client->enable_server_certificate_verification(true);
#endif

            httplib::Request req;
            req.method = method;
            req.path = Utils::getPathFromUrl(url);

            for (const auto& [key, value] : headers) {
                req.headers.emplace(key, value);
            }

            // Multipart logic
            if (!multipart.empty()) {
                std::string boundary = make_boundary();
                std::string mp_body = build_multipart_body(multipart, boundary);
                req.set_header("Content-Type", "multipart/form-data; boundary=" + boundary);
                req.set_header("Content-Length", std::to_string(mp_body.size()));
                req.body = std::move(mp_body);
            } else {
                req.body = body;
            }

            // Streaming callback logic
            if (write_cb) {
                req.response_handler = [](const httplib::Response &) { return true; };
                req.content_receiver = [write_cb](const char *data, size_t len, uint64_t, uint64_t) {
                    return write_cb(std::string_view(data, len));
                };
            }

            auto res = client->send(req);

            // Redirect handling (Recursive logic adapted for pointer usage)
            if (res && (res->status >= 301 && res->status <= 308)) {
                auto location = res->get_header_value("Location");
                if (!location.empty()) {
                    // Redirects usually require a NEW client because the host changes
                    // We do not modify the 'persistent' client here, we make a temp one.
                    auto redirect_client = std::make_unique<httplib::Client>(Utils::getBaseUrl(location));
                    redirect_client->set_connection_timeout(timeout_ms / 1000);
                    redirect_client->set_read_timeout(timeout_ms / 1000);

                    // Recursive call? Or just simple GET?
                    // Original code did a simple GET on redirect.
                    res = redirect_client->Get(Utils::getPathFromUrl(location), req.headers);
                }
            }

            if (res) {
                result.status_code = res->status;
                result.headers = res->headers;
                if (!write_cb) result.text = res->body;
                result.error = false;
                result.error_message = res->reason;
            } else {
                result.error = true;
                result.error_message = to_string(res.error());
                result.status_code = -1;
            }

        } catch (const std::exception& e) {
            result.error = true;
            result.error_message = e.what();
            result.status_code = -1;
        }
        return result;
    }

    // --- Helpers ---
    static std::string make_boundary() {
        static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<size_t> dist(0, sizeof(alphanum) - 2);
        std::string s = "----cpp-httplib-boundary-";
        for (int i = 0; i < 24; ++i) s += alphanum[dist(rng)];
        return s;
    }

    static std::string build_multipart_body(const std::vector<httplib::MultipartFormData>& parts, const std::string& boundary) {
        std::string body;
        body.reserve(1024);
        for (const auto& p : parts) {
            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"" + p.name + "\"";
            if (!p.filename.empty()) body += "; filename=\"" + p.filename + "\"";
            body += "\r\n";
            if (!p.content_type.empty()) body += "Content-Type: " + p.content_type + "\r\n";
            body += "\r\n" + p.content + "\r\n";
        }
        body += "--" + boundary + "--\r\n";
        return body;
    }
};

} // namespace agents
