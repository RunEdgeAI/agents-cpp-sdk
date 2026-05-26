/**
 * @file prompt_chaining_workflow.h
 * @brief Prompt Chaining Workflow Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/workflow.h>
#include <functional>
#include <vector>

namespace agents {
namespace workflows {

/**
 * @brief A workflow that chains multiple prompts together
 *
 * This workflow decomposes a task into a sequence of steps, where each LLM call
 * processes the output of the previous one. It can add programmatic checks between
 * steps to ensure the process is on track.
 */
class PromptChainingWorkflow : public Workflow {
public:
    /**
     * @brief Step in the workflow
     */
    struct Step {
        /**
         * @brief Name of the step
         */
        std::string name;

        /**
         * @brief Prompt template for this step
         */
        std::string prompt_template;

        /**
         * @brief Function to validate step output (returns true if valid)
         */
        std::function<bool(const JsonObject&)> validator;

        /**
         * @brief Function to transform step output for the next step
         */
        std::function<JsonObject(const JsonObject&)> transformer;

        /**
         * @brief Flag to control tool usage
         */
        bool use_tools = false;

        /**
         * @brief Constructor
         * @param name The name of the step
         * @param prompt_template The prompt template of the step
         * @param validator The validator function for the step
         * @param transformer The transformer function for the step
         */
        Step(
            const std::string& name,
            const std::string& prompt_template,
            std::function<bool(const JsonObject&)> validator = nullptr,
            std::function<JsonObject(const JsonObject&)> transformer = nullptr
        ) : name(name), prompt_template(prompt_template),
            validator(validator), transformer(transformer) {}
    };

    /**
     * @brief Constructor with context
     * @param context The context to use
     */
    PromptChainingWorkflow(std::shared_ptr<Context> context);

    /**
     * @brief Destructor
     */
    virtual ~PromptChainingWorkflow() = default;

    /**
     * @brief Add a step to the workflow
     * @param step The step to add
     */
    void addStep(const Step& step);

    /**
     * @brief Add a step to the workflow with basic params
     * @param name The name of the step
     * @param prompt_template The prompt template for the step
     * @param validator The validator function for the step
     * @param transformer The transformer function for the step
     */
    void addStep(
        const std::string& name,
        const std::string& prompt_template,
        std::function<bool(const JsonObject&)> validator = nullptr,
        std::function<JsonObject(const JsonObject&)> transformer = nullptr
    );

    /**
     * @brief Execute the workflow with input
     * @param input The input to execute the workflow with
     * @return The result of the workflow execution
     */
    JsonObject run(const std::string& input) override;

    /**
     * @brief Execute the workflow using the latest USER message from context memory
     * @return The result of the workflow execution
     */
    JsonObject run();

private:
    /**
     * @brief List of steps in the workflow
     */
    std::vector<Step> steps_;

    /**
     * @brief Step outputs record
     */
    JsonObject step_outputs_;

    /**
     * @brief Execute the workflow with input
     * @param input The input to execute the workflow with
     * @return The result of the workflow execution
     */
    JsonObject runChain(const JsonObject& input);
};

} // namespace workflows
} // namespace agents
