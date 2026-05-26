/**
 * @file agent.h
 * @brief Agent Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/context.h>
#include <agents-cpp/coroutine_utils.h>
#include <agents-cpp/types.h>
#include <functional>
#include <memory>

namespace agents {

/**
 * @brief Interface for agents
 *
 * @details Agents are LLM-powered systems that can use tools and make decisions
 * to accomplish a task.
 */
class Agent {
public:
    /**
     * @brief Agent execution state
     */
    enum class State {
        /**
         * @brief Ready to start execution
         */
        READY,
        /**
         * @brief Currently executing
         */
        RUNNING,
        /**
         * @brief Waiting for human input
         */
        WAITING,
        /**
         * @brief Execution completed successfully
         */
        COMPLETED,
        /**
         * @brief Execution failed
         */
        FAILED,
        /**
         * @brief Execution stopped by user
         */
        STOPPED
    };

    /**
     * @brief Agent execution options
     */
    struct Options {
        /**
         * @brief The maximum number of iterations
         */
        int max_iterations = 10;

        /**
         * @brief The maximum number of consecutive errors
         */
        int max_consecutive_errors = 3;

        /**
         * @brief Whether human feedback is enabled
         */
        bool human_feedback_enabled = false;

        /**
         * @brief The human in the loop function
         * @param message The message to approve
         * @param context The context
         * @param modifications Output parameter for human modifications
         * @return Whether the step was approved
         */
        std::function<bool(const std::string&, const JsonObject&, std::string&)> human_in_the_loop;
    };

    /**
     * @brief Constructor
     * @param context The agent context
     */
    Agent(std::shared_ptr<Context> context);

    /**
     * @brief Destructor
     */
    virtual ~Agent() = default;

    /**
     * @brief Initialize the agent
     */
    virtual void init() = 0;

    /**
     * @brief Run the agent with a task using coroutines
     * @param task The task to run
     * @return The result of the task
     */
    virtual Task<JsonObject> run(const std::string& task) = 0;

    /**
     * @brief Run the agent with a callback
     * @param task The task to run
     * @param callback The callback to run
     */
    virtual void runAsync(const std::string& task, std::function<void(const JsonObject&)> callback);

    /**
     * @brief Stop the agent
     */
    virtual void stop();

    /**
     * @brief Get the agent's context
     * @return The agent context
     */
    std::shared_ptr<Context> getContext() const;

    /**
     * @brief Get the agent's current state
     * @return The agent state
     */
    State getState() const;

    /**
     * @brief Set execution options
     * @param options The execution options
     */
    void setOptions(const Options& options);

    /**
     * @brief Get execution options
     * @return The execution options
     */
    const Options& getOptions() const;

    /**
     * @brief Set a callback for status updates
     * @param callback The callback to set
     */
    void setStatusCallback(std::function<void(const std::string&)> callback);

    /**
     * @brief Provide human feedback
     * @param feedback The feedback to provide
     */
    virtual void provideFeedback(const std::string& feedback);

    /**
     * @brief Wait for feedback using coroutines
     * @param message The message to wait for feedback
     * @param context The context to wait for feedback
     * @return The feedback
     */
    virtual Task<std::string> waitForFeedback(
        const std::string& message,
        const JsonObject& context
    ) {
        (void)message;
        (void)context;
        // Placeholder implementation, to be overridden
        co_return "";
    }

/*! @cond PRIVATE */
protected:
    /**
     * @brief The agent context
     */
    std::shared_ptr<Context> context_;

    /**
     * @brief The agent state
     */
    State state_ = State::READY;

    /**
     * @brief The agent options
     */
    Options options_;

    /**
     * @brief The agent status callback
     * @note This is a callback for status updates
     */
    std::function<void(const std::string&)> status_callback_;

    /**
     * @brief Update the agent's state
     * @param state The state to set
     */
    void setState(State state);

    /**
     * @brief Log a status message
     * @param status The status to log
     */
    void logStatus(const std::string& status);
/*! @endcond */
};

} // namespace agents
