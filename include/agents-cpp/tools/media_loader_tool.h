/**
 * @file media_loader_tool.h
 * @brief Media Loader Tool Definition
 * @version 0.1
 * @date 2025-09-22
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/llm_interface.h>
#include <agents-cpp/tool.h>
#include <agents-cpp/types.h>

#include <memory>
#include <string>
#include <map>

namespace agents {
namespace tools {

/**
 * @brief Media loader tool that loads media from URLs and returns them as base64 encoded data URLs
 */
class MediaLoaderTool : public Tool {
public:
    /**
     * @brief Construct a new Image Loader Tool object
     * @param llm The LLM interface to use
     */
    MediaLoaderTool(std::shared_ptr<LLMInterface> llm);

    /**
     * @brief Execute the Image Loader Tool
     * @param params The parameters for the Image Loader Tool
     * @return ToolResult The result of the Image Loader Tool
     */
    ToolResult execute(const JsonObject& params) const override;

private:
    /**
     * @brief Setup the parameters for the Image Loader Tool
     */
    void setupParameters();

    /**
     * @brief Get MIME type from file extension
     * @param path The file path or URL
     * @return The MIME type string
     */
    std::string getMimeType(const std::string& path) const;

    /**
     * @brief Infer media kind from MIME string prefix
     * (image/, audio/, video/)
     * @param mime The MIME string
     * @return The media kind
     */
    inline std::string inferKindFromMime(const std::string& mime) const {
        if (mime.rfind("image/", 0) == 0) return "image";
        if (mime.rfind("audio/", 0) == 0) return "audio";
        if (mime.rfind("video/", 0) == 0) return "video";
        if (mime.rfind("application/", 0) == 0) return "document";
        return ""; // default fallback
    }

    /**
     * @brief Load image from HTTP/HTTPS URL
     * @param url The URL to load from
     * @return The binary content of the image
     */
    std::string loadFromUrl(const std::string& url) const;

    /**
     * @brief Load image from local file path
     * @param filePath The local file path
     * @return The binary content of the image
     */
    std::string loadFromFile(const std::string& filePath) const;

    /**
     * @brief Encode binary data to base64
     * @param data The binary data to encode
     * @return The base64 encoded string
     */
    std::string base64Encode(const std::string& data) const;

    /**
     * @brief Maximum media size to inline (1MB)
     */
    static const size_t max_media_size_;

    /**
     * @brief MIME type mapping for common image formats
     */
    static const std::map<std::string, std::string> mime_type_map_;

    /**
     * @brief The LLM interface to use
     */
    std::shared_ptr<LLMInterface> llm_;
};

} // namespace tools
} // namespace agents