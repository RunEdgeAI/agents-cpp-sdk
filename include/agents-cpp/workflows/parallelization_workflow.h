/**
 * @file parallelization_workflow.h
 * @brief Parallelization Workflow Definition
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
 * @brief A workflow that runs multiple tasks in parallel
 *
 * @details This workflow allows LLMs to work simultaneously on a task and have their outputs
 * aggregated. It supports two key variations:
 * - Sectioning: Breaking a task into independent subtasks run in parallel
 * - Voting: Running the same task multiple times to get diverse outputs
 */
class ParallelizationWorkflow : public Workflow {
public:
    /**
     * @brief Enum for parallelization strategy
     * @note This is an enum for the parallelization strategy. It contains the SECTIONING and VOTING strategies.
     */
    enum class Strategy {
        /**
         * @brief Break task into independent subtasks
         */
        SECTIONING,
        /**
         * @brief Run same task multiple times for consensus
         */
        VOTING
    };

    /**
     * @brief Task definition for parallel execution
     * @note This is a task in the parallelization workflow. It contains the name, prompt template, context, prompt function, and result parser.
     */
    struct Task {
        /**
         * @brief The name of the task
         */
        std::string name;
        /**
         * @brief The prompt template of the task
         */
        std::string prompt_template;
        /**
         * @brief The context of the task
         */
        JsonObject context;
        /**
         * @brief The prompt function of the task
         */
        std::function<std::string(const std::string&)> prompt_fn;
        /**
         * @brief The result parser of the task
         */
        std::function<JsonObject(const std::string&)> result_parser;

        /**
         * @brief Constructor
         * @param name The name of the task
         * @param prompt_template The prompt template of the task
         * @param context The context of the task
         * @param prompt_fn The prompt function of the task
         * @param result_parser The result parser of the task
         */
        Task(
            const std::string& name,
            const std::string& prompt_template,
            const JsonObject& context = JsonObject(),
            std::function<std::string(const std::string&)> prompt_fn = nullptr,
            std::function<JsonObject(const std::string&)> result_parser = nullptr
        ) : name(name), prompt_template(prompt_template), context(context), prompt_fn(prompt_fn), result_parser(result_parser) {}
    };

    /**
     * @brief Constructor with context
     * @param context The context of the workflow
     * @param strategy The strategy of the workflow
     */
    ParallelizationWorkflow(
        std::shared_ptr<Context> context,
        Strategy strategy = Strategy::SECTIONING
    );

    /**
     * @brief Destructor
     */
    virtual ~ParallelizationWorkflow() = default;

    /**
     * @brief Add a task to the workflow
     * @param task The task to add
     */
    void addTask(const Task& task);

    /**
     * @brief Add a task to the workflow with basic params
     * @param name The name of the task
     * @param prompt_template The prompt template of the task
     * @param context The context of the task
     */
    void addTask(
        const std::string& name,
        const std::string& prompt_template,
        const JsonObject& context = JsonObject()
    );

    /**
     * @brief Add a task to the workflow with generators and parser
     * @param name The name of the task
     * @param prompt_template The prompt template of the task
     * @param prompt_fn The prompt function of the task
     * @param result_parser The result parser of the task
     * @param context The context of the task
     */
    void addTask(
        const std::string& name,
        const std::string& prompt_template,
        std::function<std::string(const std::string&)> prompt_fn,
        std::function<JsonObject(const std::string&)> result_parser,
        const JsonObject& context = JsonObject()
    );

    /**
     * @brief Set the aggregation function for task results
     * @param aggregator The aggregation function
     */
    void setAggregator(std::function<JsonObject(const std::vector<JsonObject>&)> aggregator);

    /**
     * @brief Set the strategy
     * @param strategy The strategy
     */
    void setStrategy(Strategy strategy);

    /**
     * @brief Initialize the workflow
     */
    void init();

    /**
     * @brief Set the voting threshold (for VOTING mode)
     * @param threshold The threshold
     */
    void setVotingThreshold(double threshold);

    /**
     * @brief Execute the workflow with input
     * @param input The input to the workflow
     * @return The output of the workflow
     */
    JsonObject run(const std::string& input) override;

    /**
     * @brief Execute using the latest USER message from context memory
     * @return The output of the workflow
     */
    JsonObject run();

private:
    /**
     * @brief List of tasks to execute in parallel
     */
    std::vector<Task> tasks_;

    /**
     * @brief Strategy to use
     */
    Strategy strategy_;

    /**
     * @brief Aggregation function for results
     */
    std::function<JsonObject(const std::vector<JsonObject>&)> aggregator_;

    /**
     * @brief Voting threshold
     */
    double voting_threshold_ = 0.5;

    /**
     * @brief Cached pointer to the LLM from context for direct calls
     */
    std::shared_ptr<LLMInterface> llm_;

    /**
     * @brief Run tasks in parallel (implementation uses std::async)
     * @param input The input to the workflow
     * @return The output of the workflow
     */
    std::vector<JsonObject> runTasksInParallel(const std::string& input);

    /**
     * @brief Default aggregator for sectioning strategy
     * @param results The results to aggregate
     * @return The aggregated results
     */
    static JsonObject defaultSectioningAggregator(const std::vector<JsonObject>& results);

    /**
     * @brief Default aggregator for voting strategy
     * @param results The results to aggregate
     * @return The aggregated results
     */
    static JsonObject defaultVotingAggregator(const std::vector<JsonObject>& results);
};

} // namespace workflows
} // namespace agents
