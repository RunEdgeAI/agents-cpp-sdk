/**
 * @file evaluator_workflow.h
 * @brief Evaluator Workflow Definition
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
 * @brief An evaluator-optimizer workflow where one LLM optimizes output based
 * on another's feedback
 *
 * @details In this workflow, one LLM call generates a response while another provides
 * evaluation and feedback in a loop. This pattern is effective when we have
 * clear evaluation criteria and iterative refinement provides value.
 */
class EvaluatorWorkflow : public Workflow {
public:
    /**
     * @brief Constructor with LLM interfaces for optimizer and evaluator
     * @param context The context to use
     * @param optimizer_prompt_template The optimizer prompt template
     * @param evaluator_prompt_template The evaluator prompt template
     */
    EvaluatorWorkflow(
        std::shared_ptr<Context> context,
        const std::string& optimizer_prompt_template = "",
        const std::string& evaluator_prompt_template = ""
    );

    /**
     * @brief Initialize the workflow
     */
    void init();

    /**
     * @brief Execute the workflow with input
     * @param input The input to execute the workflow with
     * @return The result of the workflow execution
     */
    JsonObject run(const std::string& input) override;

    /**
     * @brief Set the evaluation criteria for the evaluator
     * @param criteria The evaluation criteria to set
     */
    void setEvaluationCriteria(const std::vector<std::string>& criteria);

    /**
     * @brief Set the max number of feedback iterations
     * @param max_iterations The max number of feedback iterations to set
     */
    void setMaxIterations(int max_iterations);

    /**
     * @brief Set the improvement threshold (minimum score to accept a response)
     * @param threshold The improvement threshold to set
     */
    void setImprovementThreshold(double threshold);

    /**
     * @brief Set the minimum acceptable score
     * @param threshold The minimum acceptable score to set
     */
    void setMinimumAcceptableScore(double threshold) { setImprovementThreshold(threshold); }

    /**
     * @brief Set the optimizer prompt template
     * @param prompt_template The optimizer prompt template to set
     */
    void setOptimizerPromptTemplate(const std::string& prompt_template);

    /**
     * @brief Set the optimizer prompt
     * @param prompt_template The optimizer prompt to set
     */
    void setOptimizerPrompt(const std::string& prompt_template) { setOptimizerPromptTemplate(prompt_template); }

    /**
     * @brief Set the evaluator prompt template
     * @param prompt_template The evaluator prompt template to set
     */
    void setEvaluatorPromptTemplate(const std::string& prompt_template);

    /**
     * @brief Set the evaluator prompt
     * @param prompt_template The evaluator prompt to set
     */
    void setEvaluatorPrompt(const std::string& prompt_template) { setEvaluatorPromptTemplate(prompt_template); }

    /**
     * @brief Set the optimizer function
     * @param optimizer The optimizer function to set
     */
    void setOptimizer(std::function<std::string(const std::string&, const JsonObject&)> optimizer);

    /**
     * @brief Set the evaluator function
     * @param evaluator The evaluator function to set
     */
    void setEvaluator(std::function<JsonObject(const std::string&, const std::string&)> evaluator);

private:
    /**
     * @brief LLM for the evaluator (if nullptr, use the main LLM)
     */
    std::shared_ptr<LLMInterface> evaluator_llm_;

    /**
     * @brief Prompt templates
     */
    std::string optimizer_prompt_template_;
    std::string evaluator_prompt_template_;

    /**
     * @brief Evaluation criteria
     */
    std::vector<std::string> evaluation_criteria_;

    /**
     * @brief Max iterations for the feedback loop
     */
    int max_iterations_ = 3;

    /**
     * @brief Improvement threshold for accepting a response
     */
    double improvement_threshold_ = 0.8;  // 0.0 to 1.0

    /**
     * @brief Optimizer function
     */
    std::function<std::string(const std::string&, const JsonObject&)> optimizer_;

    /**
     * @brief Evaluator function
     * @note This is the function that evaluates the output of the optimizer
     */
    std::function<JsonObject(const std::string&, const std::string&)> evaluator_;

    /**
     * @brief Default optimizer function
     */
    std::string defaultOptimizer(const std::string& input, const JsonObject& feedback);

    /**
     * @brief Default evaluator function
     */
    JsonObject defaultEvaluator(const std::string& input, const std::string& output);

    /**
     * @brief Create the evaluator system prompt with criteria
     * @return The evaluator system prompt
     */
    std::string createEvaluatorSystemPrompt() const;
};

} // namespace workflows
} // namespace agents
