/**
 * @file routing_workflow.h
 * @brief Routing Workflow Definition
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

namespace agents {
namespace workflows {

/**
 * @brief Routing workflow using the actor model
 *
 * @details This workflow classifies an input and directs it to a specialized follow-up
 * task or handler. It allows for separation of concerns and building more
 * specialized prompts for different types of inputs.
 */
class RoutingWorkflow : public Workflow {
public:
    /**
     * @brief Handler definition for a route
     * @note This is a handler for a route. It contains the name, description, prompt template, LLM, workflow, and handler.
     */
    struct RouteHandler {
        /**
         * @brief The name of the route handler
         */
        std::string name;
        /**
         * @brief The description of the route handler
         */
        std::string description;
        /**
         * @brief The prompt template of the route handler
         */
        std::string prompt_template;
        /**
         * @brief The LLM of the route handler
         */
        std::shared_ptr<LLMInterface> llm;  // Optional separate LLM for this route
        /**
         * @brief The workflow of the route handler
         */
        std::shared_ptr<Workflow> workflow;  // Optional workflow to delegate to
        /**
         * @brief The handler of the route handler
         */
        std::function<JsonObject(const std::string&, const JsonObject&)> handler; // Optional function handler

        /**
         * @brief Constructor
         * @param name The name of the route handler
         * @param description The description of the route handler
         * @param prompt_template The prompt template of the route handler
         * @param llm The LLM of the route handler
         * @param workflow The workflow of the route handler
         * @param handler The handler of the route handler
         */
        RouteHandler(
            const std::string& name,
            const std::string& description,
            const std::string& prompt_template = "",
            std::shared_ptr<LLMInterface> llm = nullptr,
            std::shared_ptr<Workflow> workflow = nullptr,
            std::function<JsonObject(const std::string&, const JsonObject&)> handler = nullptr
        ) : name(name), description(description),
            prompt_template(prompt_template), llm(llm), workflow(workflow), handler(handler) {}
    };

    /**
     * @brief Constructor with context and router prompt template
     * @param context The context of the workflow
     * @param router_prompt_template The router prompt template
     */
    RoutingWorkflow(
        std::shared_ptr<Context> context,
        const std::string& router_prompt_template = ""
    );

    /**
     * @brief Add a route handler
     * @param handler The handler to add
     */
    void addRouteHandler(const RouteHandler& handler);

    /**
     * @brief Add a route handler with basic params
     * @param name The name of the route
     * @param description The description of the route
     * @param prompt_template The prompt template of the route
     * @param handler_llm The LLM of the route
     * @param workflow The workflow of the route
     */
    void addRouteHandler(
        const std::string& name,
        const std::string& description,
        const std::string& prompt_template = "",
        std::shared_ptr<LLMInterface> handler_llm = nullptr,
        std::shared_ptr<Workflow> workflow = nullptr
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
     * @brief Set the router prompt template
     * @param prompt_template The prompt template to set
     */
    void setRouterPromptTemplate(const std::string& prompt_template);

    /**
     * @brief Set the router prompt
     * @param prompt_template The prompt template to set
     */
    void setRouterPrompt(const std::string& prompt_template) { setRouterPromptTemplate(prompt_template); }

    /**
     * @brief Set default handler for unknown routes
     * @param handler The handler to set
     */
    void setDefaultHandler(const RouteHandler& handler);

    /**
     * @brief Set default route
     * @param handler The handler to set
     */
    void setDefaultRoute(std::function<JsonObject(const std::string&, const JsonObject&)> handler);

    /**
     * @brief Define available routes as a JSON schema
     * @return The schema of the routes
     */
    JsonObject getRoutesSchema() const;

    /**
     * @brief Add a route with a direct function handler
     * @param name The name of the route
     * @param description The description of the route
     * @param handler The handler of the route
     */
    void addRoute(
        const std::string& name,
        const std::string& description,
        std::function<JsonObject(const std::string&, const JsonObject&)> handler
    );

private:
    /**
     * @brief Router prompt template
     */
    std::string router_prompt_template_;

    /**
     * @brief Map of route handlers
     */
    std::map<std::string, RouteHandler> route_handlers_;

    /**
     * @brief Default handler for unknown routes
     */
    std::optional<RouteHandler> default_handler_;

    /**
     * @brief Create the router system prompt with route descriptions
     * @return The router system prompt
     */
    std::string createRouterSystemPrompt() const;
};

} // namespace workflows
} // namespace agents
