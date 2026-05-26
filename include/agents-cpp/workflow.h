/**
 * @file workflow.h
 * @brief Workflow Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/context.h>
#include <agents-cpp/types.h>
#include <functional>
#include <memory>

namespace agents {
/**
 * @brief Worflows Namespace
 *
 */
namespace workflows {

/**
 * @brief Abstract base class for workflows
 *
 * A workflow is a pattern for executing a series of
 * LLM operations to accomplish a task.
 */
class Workflow {
public:
    /**
     * @brief Constructor
     * @param context The context
     */
    Workflow(std::shared_ptr<Context> context);

    /**
     * @brief Destructor
     */
    ~Workflow() = default;

    /**
     * @brief Run the workflow with a user input and return the result
     * @param input The user input
     * @return The result
     */
    virtual JsonObject run(const std::string& input) = 0;

    /**
     * @brief Run the workflow with a user input asynchronously
     * @param input The user input
     * @param callback The callback
     */
    virtual void runAsync(
        const std::string& input,
        std::function<void(const JsonObject&)> callback
    );

    /**
     * @brief Get the workflow's context
     * @return The context
     */
    std::shared_ptr<Context> getContext() const;

    /**
     * @brief Set a callback for intermediate steps
     * @param callback The callback
     */
    void setStepCallback(std::function<void(const std::string&, const JsonObject&)> callback);

    /**
     * @brief Set maximum number of steps
     * @param max_steps The maximum number of steps
     */
    void setMaxSteps(int max_steps);

    /**
     * @brief Get maximum number of steps
     * @return The maximum number of steps
     */
    int getMaxSteps() const;

/*! @cond PRIVATE */
protected:
    /**
     * @brief The context
     */
    std::shared_ptr<Context> context_;

    /**
     * @brief The step callback
     */
    std::function<void(const std::string&, const JsonObject&)> step_callback_;

    /**
     * @brief The maximum number of steps
     */
    int max_steps_ = 10;

    /**
     * @brief Log a step with description and result
     * @param description The description
     * @param result The result
     */
    void logStep(const std::string& description, const JsonObject& result);
/*! @endcond */
};

} // namespace workflows
} // namespace agents
