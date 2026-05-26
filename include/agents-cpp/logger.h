/**
 * @file logger.h
 * @brief Logger Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <cstdio>
#include <memory>
#include <string>

namespace agents {

/**
 * @brief Logger utility class that wraps spdlog functionality
 */
class Logger {
public:
    /**
     * @brief The log level
     */
    enum class Level {
        /**
         * @brief Trace logging level
         */
        TRACE,
        /**
         * @brief Debug logging level
         */
        DEBUG,
        /**
         * @brief Info logging level
         */
        INFO,
        /**
         * @brief Warning logging level
         */
        WARN,
        /**
         * @brief Error logging level
         */
        ERR,
        /**
         * @brief Critical logging level
         */
        CRITICAL,
        /**
         * @brief Logging disabled level
         */
        OFF
    };

    /**
     * @brief Initialize the logger
     * @param level The log level
     */
    static void init(Level level = Level::INFO);

    /**
     * @brief Set the log level
     * @param level The log level
     */
    static void setLevel(Level level);

    /**
     * @brief Convert Level enum to spdlog level
     * @param level The log level
     * @return The spdlog level
     */
    static spdlog::level::level_enum toSpdlogLevel(Level level);

    /**
     * @brief Log a message at trace level
     * @param fmt The format string
     * @param args The arguments
     */
    template<typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::trace(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a message at debug level
     * @param fmt The format string
     * @param args The arguments
     */
    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::debug(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a message at info level
     * @param fmt The format string
     * @param args The arguments
     */
    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a message at warn level
     * @param fmt The format string
     * @param args The arguments
     */
    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a message at error level
     * @param fmt The format string
     * @param args The arguments
     */
    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a message at critical level
     * @param fmt The format string
     * @param args The arguments
     */
    template<typename... Args>
    static void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::critical(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Write a raw chunk of text to stdout without any log prefix or newline.
     *        Flushes immediately — intended for streaming token output.
     *        Respects the current log level: suppressed when level > INFO.
     * @param text The text chunk to write
     */
    static void chunk(const std::string& text) {
        if (spdlog::get_level() <= spdlog::level::info) {
            std::fputs(text.c_str(), stdout);
            std::fflush(stdout);
        }
    }

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

} // namespace agents
