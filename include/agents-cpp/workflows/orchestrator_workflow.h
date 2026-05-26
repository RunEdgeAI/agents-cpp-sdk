/**
 * @file orchestrator_workflow.h
 * @brief Orchestrator Workflow Definition
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/workflow.h>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace agents {
namespace workflows {

/**
 * @brief A workflow where a central orchestrator delegates tasks to workers
 *
 * In this workflow, a central LLM (orchestrator) dynamically breaks down tasks,
 * delegates them to worker LLMs, and synthesizes their results. This is well-suited
 * for complex tasks where subtasks can't be predetermined.
 */
class OrchestratorWorkflow : public Workflow {
public:
    /**
     * @brief Worker definition
     * @note This is a worker in the orchestrator workflow. It contains the name, description,
     * prompt template, llm, and handler.
     */
    struct Worker {
        /**
         * @brief The name of the worker
         */
        std::string name;
        /**
         * @brief The description of the worker
         */
        std::string description;
        /**
         * @brief The prompt template of the worker
         */
        std::string prompt_template;
        /**
         * @brief The llm of the worker
         */
        std::shared_ptr<LLMInterface> llm;
        /**
         * @brief The handler of the worker
         */
        std::function<JsonObject(const std::string&, const JsonObject&)> handler;

        /**
         * @brief Constructor
         * @param name The name of the worker
         * @param description The description of the worker
         * @param prompt_template The prompt template of the worker
         * @param llm The llm of the worker
         * @param handler The handler of the worker
         */
        Worker(
            const std::string& name,
            const std::string& description,
            const std::string& prompt_template,
            std::shared_ptr<LLMInterface> llm = nullptr,
            std::function<JsonObject(const std::string&, const JsonObject&)> handler = nullptr
        ) : name(name), description(description),
            prompt_template(prompt_template), llm(llm), handler(handler) {}
    };

    /**
     * @brief Constructor with context and orchestrator prompt
     * @param context The context of the workflow
     * @param orchestrator_prompt_template The orchestrator prompt template
     */
    OrchestratorWorkflow(
        std::shared_ptr<Context> context,
        const std::string& orchestrator_prompt_template = ""
    );

    /**
     * @brief Add a worker to the workflow
     * @param worker The worker to add
     */
    void addWorker(const Worker& worker);

    /**
     * @brief Add a worker with basic params
     * @param name The name of the worker
     * @param description The description of the worker
     * @param prompt_template The prompt template of the worker
     * @param worker_llm The llm of the worker
     */
    void addWorker(
        const std::string& name,
        const std::string& description,
        const std::string& prompt_template,
        std::shared_ptr<LLMInterface> worker_llm = nullptr
    );

    /**
     * @brief Initialize the workflow
     */
    void init();

    /**
     * @brief Execute the workflow with input
     * @param input The input to the workflow
     * @return The output of the workflow
     */
    JsonObject run(const std::string& input) override;

    /**
     * @brief Set the max number of iterations
     * @param max_iterations The max number of iterations
     */
    void setMaxIterations(int max_iterations);

    /**
     * @brief Set the result synthesizer function
     * @param synthesizer The synthesizer function
     */
    void setSynthesizer(std::function<JsonObject(const std::vector<JsonObject>&)> synthesizer);

    /**
     * @brief Set the orchestrator prompt template
     * @param orchestrator_prompt_template The orchestrator prompt template
     */
    void setOrchestratorPrompt(const std::string& orchestrator_prompt_template);

    /**
     * @brief Get the schema for available workers
     * @return The schema for available workers
     */
    JsonObject getWorkersSchema() const;

private:
    /**
     * @brief Orchestrator prompt template
     */
    std::string orchestrator_prompt_template_;

    /**
     * @brief List of available workers
     */
    std::vector<Worker> workers_;

    /**
     * @brief Synthesizer function
     */
    std::function<JsonObject(const std::vector<JsonObject>&)> synthesizer_;

    /**
     * @brief Map of registered workers by name for quick lookup
     */
    std::map<std::string, Worker> worker_map_;

    /**
     * @brief Max number of iterations
     */
    int max_iterations_ = 5;

    /**
     * @brief Default synthesizer function
     */
    JsonObject defaultSynthesizer(const std::vector<JsonObject>& results);

    /**
     * @brief Create the orchestrator system prompt with worker descriptions
     * @return The orchestrator system prompt
     */
    std::string createOrchestratorSystemPrompt() const;

    /**
     * @brief Execute a worker by name
     */
    JsonObject executeWorker(
        const std::string& worker_name,
        const std::string& task,
        const JsonObject& context_data
    );
};

} // namespace workflows
} // namespace agents
