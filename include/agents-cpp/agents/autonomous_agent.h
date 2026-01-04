/**
 * @file autonomous_agent.h
 * @brief Autonomous Agent Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/agent.h>
#include <agents-cpp/coroutine_utils.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <regex>
#include <thread>

namespace agents {

/**
 * @brief An agent that operates autonomously to complete a task
 *
 * Autonomous agents start with a task, plan steps to accomplish it,
 * and use tools to execute those steps. They can be configured with
 * various strategies and human-in-the-loop options.
 */
class AutonomousAgent : public Agent {
public:
    /**
     * @brief Step in the agent's execution
     */
    struct Step {
        /**
         * @brief The description of the step
         */
        std::string description;
        /**
         * @brief The status of the step
         */
        std::string status;
        /**
         * @brief The result of the step
         */
        JsonObject result;
        /**
         * @brief Whether the step was successful
         */
        bool success;
    };

    /**
     * @brief Planning strategy for the agent
     */
    enum class PlanningStrategy {
        /**
         * @brief Generate actions without explicit planning
         */
        ZERO_SHOT,
        /**
         * @brief Generate multiple reasoning paths
         */
        TREE_OF_THOUGHT,
        /**
         * @brief Generate a plan then execute it
         */
        PLAN_AND_EXECUTE,
        /**
         * @brief Reflect on past steps for improvement
         */
        REFLEXION,
        /**
         * @brief Reasoning and acting
         */
        REACT
    };

    /**
     * @brief Constructor
     * @param context The agent context
     */
    AutonomousAgent(std::shared_ptr<Context> context);

    /**
     * @brief Destructor
     */
    ~AutonomousAgent() override = default;

    /**
     * @brief Initialize the agent
     */
    void init() override;

    /**
     * @brief Run the agent with a task using coroutines
     * @param task The task to run
     * @return The result of the task
     */
    Task<JsonObject> run(const std::string& task) override;

    /**
     * @brief Run the agent with a callback
     * @param task The task to run
     * @param callback The callback for incremental updates
     */
    void runAsync(const std::string& task, std::function<void(const JsonObject&)> callback) override;

    /**
     * @brief Stop the agent
     */
    void stop() override;

    /**
     * @brief Provide human feedback
     * @param feedback The feedback
     */
    void provideFeedback(const std::string& feedback) override;

    /**
     * @brief Set the agent prompt
     * @param agent_prompt The agent prompt
     */
    void setAgentPrompt(const std::string& agent_prompt);

    /**
     * @brief Set the planning strategy
     * @param strategy The planning strategy
     */
    void setPlanningStrategy(PlanningStrategy strategy);

    /**
     * @brief Get the steps executed so far
     * @return The steps executed so far
     */
    std::vector<Step> getSteps() const;

    /**
     * @brief Set a callback for when a step is completed
     * @param callback The callback
     */
    void setStepCallback(std::function<void(const Step&)> callback);

    /**
     * @brief Wait for feedback using coroutines
     * @param message The message to wait for feedback
     * @param context The context
     * @return The feedback
     */
    Task<std::string> waitForFeedback(const std::string& message, const JsonObject& context) override;

protected:
    /**
     * @brief The agent prompt
     */
    std::string agent_prompt_;
    /**
     * @brief Planning strategy for the agent
     */
    PlanningStrategy planning_strategy_ = PlanningStrategy::REACT;
    /**
     * @brief The steps executed so far
     */
    std::vector<Step> steps_;
    /**
     * @brief Callback for when a step is completed
     */
    std::function<void(const Step&)> step_callback_;
    /**
     * @brief Callback for task's incremental updates
     */
    std::function<void(const JsonObject&)> task_callback_;
    /**
     * @brief Execution state flag to indicate if the agent should stop
     */
    std::atomic<bool> should_stop_{false};
    /**
     * @brief Promise for coroutine-based feedback
     */
    std::promise<std::string> feedback_promise_;

    /**
     * @brief Execute the agent's task using coroutines
     * @param task The task to execute
     * @return The result of the task
     */
    Task<JsonObject> executeTask(const std::string& task);

    /**
     * @brief Execute a step using coroutines
     * @param step_description The description of the step
     * @param context The context
     * @return The result of the step
     */
    Task<Step> executeStep(std::string& step_description, JsonObject& context);

    /**
     * @brief Record a completed step
     * @param step The step to record
     */
    void recordStep(const Step& step);

    /**
     * @brief Get tool descriptions for prompts
     * @return The tool descriptions
     */
    std::string getToolDescriptions() const;

    /**
     * @brief Plan using Zero Shot
     * @param task The task to plan
     * @return The plan
     */
    Task<JsonObject> planZeroShot(const std::string& task);

    /**
     * @brief Plan using Tree of Thought
     * @param task The task to plan
     * @return The plan
     */
    Task<JsonObject> planTreeOfThought(const std::string& task);

    /**
     * @brief Plan and execute
     * @param task The task to plan
     * @return The plan
     */
    Task<JsonObject> planAndExecute(const std::string& task);

    /**
     * @brief Plan using Reflexion
     * @param task The task to plan
     * @return The plan
     */
    Task<JsonObject> planReflexion(const std::string& task);

    /**
     * @brief Plan using React
     * @param task The task to plan
     * @return The plan
     */
    Task<JsonObject> planReact(const std::string& task);
};

} // namespace agents
