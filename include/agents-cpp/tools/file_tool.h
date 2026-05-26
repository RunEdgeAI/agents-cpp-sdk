/**
 * @file file_tool.h
 * @brief File Operations Tool Header
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
 * @brief File read tool that provides secure file reading capabilities
 */
class FileReadTool : public Tool {
public:
    /**
     * @brief Construct a new File Read Tool object
     */
    FileReadTool();

    /**
     * @brief Execute the file read tool
     * @param params The parameters for the file read tool
     * @return ToolResult The result of the file read tool
     */
    ToolResult execute(const JsonObject& params) const override;

/*! @cond PRIVATE */
protected:
    /**
     * @brief Setup the parameters for the file read tool
     */
    void setupParameters();

    /**
     * @brief Validate the file path
     * @param filePath The file path to validate
     * @return true if the file path is valid, false otherwise
     */
    bool validateFilePath(const std::string& filePath) const;

    /**
     * @brief Check if the file is accessible
     * @param filePath The file path to check
     * @return true if the file is accessible, false otherwise
     */
    bool checkFileAccessibility(const std::string& filePath) const;

    /**
     * @brief Check the size of the file
     * @param filePath The file path to check
     * @param fileSize The size of the file
     * @return true if the file size is valid, false otherwise
     */
    bool checkFileSize(const std::string& filePath, std::streamsize& fileSize) const;

    /**
     * @brief Read the content of the file
     * @param filePath The file path to read
     * @return The content of the file
     */
    std::string readFileContent(const std::string& filePath) const;

    /**
     * @brief Format the file read result
     * @param filePath The file path to read
     * @param content The content of the file
     * @param fileSize The size of the file
     * @return ToolResult The result of the file read tool
     */
    ToolResult formatFileReadResult(const std::string& filePath, const std::string& content, std::streamsize fileSize) const;
/*! @endcond */
};

/**
 * @brief File write tool that provides secure file writing capabilities
 */
class FileWriteTool : public Tool {
public:
    /**
     * @brief Construct a new File Write Tool object
     */
    FileWriteTool();

    /**
     * @brief Execute the file write tool
     *
     * @param params The parameters for the file write tool
     * @return ToolResult The result of the file write tool
     */
    ToolResult execute(const JsonObject& params) const override;

/*! @cond PRIVATE */
protected:
    /**
     * @brief Setup the parameters for the file write tool
     */
    void setupParameters();

    /**
     * @brief Validate the file path
     * @param filePath The file path to validate
     * @return true if the file path is valid, false otherwise
     */
    bool validateFilePath(const std::string& filePath) const;

    /**
     * @brief Validate the content
     * @param content The content to validate
     * @return true if the content is valid, false otherwise
     */
    bool validateContent(const std::string& content) const;

    /**
     * @brief Ensure the directory exists
     * @param filePath The file path to ensure the directory exists
     * @return true if the directory exists, false otherwise
     */
    bool ensureDirectoryExists(const std::string& filePath) const;

    /**
     * @brief Write the content to the file
     * @param filePath The file path to write to
     * @param content The content to write to the file
     * @return true if the content is written to the file, false otherwise
     */
    bool writeFileContent(const std::string& filePath, const std::string& content) const;

    /**
     * @brief Verify the file write
     * @param filePath The file path to verify
     * @param writtenSize The size of the content written to the file
     * @return true if the file write is verified, false otherwise
     */
    bool verifyFileWrite(const std::string& filePath, std::streamsize& writtenSize) const;

    /**
     * @brief Format the file write result
     * @param filePath The file path to write to
     * @param content The content to write to the file
     * @param writtenSize The size of the content written to the file
     * @return ToolResult The result of the file write tool
     */
    ToolResult formatFileWriteResult(const std::string& filePath, const std::string& content, std::streamsize writtenSize) const;
/*! @endcond */
};

} // namespace tools
} // namespace agents
