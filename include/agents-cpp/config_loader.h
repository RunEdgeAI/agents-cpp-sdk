/**
 * @file config_loader.h
 * @brief Config Loader Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <string>
#include <unordered_map>

namespace agents {

/**
 * @brief A utility class to load and access configuration values from .env files
 *
 * The ConfigLoader reads configuration from a .env file and provides access to
 * these values. It automatically falls back to environment variables if a key
 * is not found in the config file.
 */
class ConfigLoader {
public:
    /**
     * @brief Construct a ConfigLoader with default .env file locations
     */
    ConfigLoader();

    /**
     * @brief Construct a ConfigLoader with a custom .env file path
     * @param customPath Path to the .env file
     */
    explicit ConfigLoader(const std::string& customPath);

    /**
     * @brief Get a configuration value
     * @param key The key to look up
     * @param defaultValue Value to return if key is not found
     * @return The value for the key, or defaultValue if not found
     */
    std::string get(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief Check if a key exists in the configuration
     * @param key The key to check
     * @return true if the key exists, false otherwise
     */
    bool has(const std::string& key) const;

    /**
     * @brief Get the singleton instance of ConfigLoader
     * @return Reference to the global ConfigLoader instance
     */
    static ConfigLoader& getInstance();

private:
    std::string envFilePath;
    std::unordered_map<std::string, std::string> config;

    void loadEnvFile();
    std::string findEnvFile();
};

} // namespace agents