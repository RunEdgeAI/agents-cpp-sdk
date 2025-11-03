[TOC]
# 🧠 Agents-SDK - A High Performance C++ Framework for AI Agents

![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)
![macOS](https://img.shields.io/badge/macOS-000000?logo=apple&logoColor=F0F0F0)
![Windows](https://custom-icon-badges.demolab.com/badge/Windows-0078D6?logo=windows11&logoColor=white)

**Agents-SDK** is a **portable, high-performance C++ framework** for building **on-device, agentic AI systems** — think **LangChain for the edge**. This SDK is purpose-built for developers who want to create **local-first AI agents** that can reason, plan, and act without relying on the cloud.

## 🚀 Features

- ⚙️ **Modular Architecture** — Compose agents from interchangeable components.
- 🧩 **Multi-LLM Support** — Connect to multiple providers seamlessly:
  - **OpenAI** (GPT-4o, GPT-4, GPT-3.5 Turbo)
  - **Anthropic** (Claude 3 family models (Opus, Sonnet, Haiku)
  - **Google** (Gemini family models (Pro, Flash)
  - **Ollama/llama-cpp** (local models like Llama, Mistral, etc.)
- ⚡ **Optimized for Speed and Memory** — Built in C++ with focus on performance.
- 🔁 **Built-In Workflow Patterns**
  - Prompt Chaining
  - Routing
  - Parallelization
  - Orchestrator-Workers
  - Evaluator-Optimizer
- 🤖 **Autonomous Agents** — Supports modern reasoning strategies:
  - ReAct (Reason + Act)
  - CoT (Chain-of-Thought)  [In Development]
  - Plan and Execute
  - Zero-Shot [In Development]
  - Reflexion [In Development]
- 🧠 **Extensible Tooling System** — Plug in your own tools or use built-in ones (Web Search, Wikipedia, Python Executor, etc).

## ⚙️ Requirements

- C++20 compatible compiler (GCC 14+, Clang 17+, MSVC 2022+)
- Bazel 8.3.1+

- Dependencies (already provided for convenience)
   - python3 (3.11+)
   - nlohmann/json
   - spdlog

## 🧭 Quick Start

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/RunEdgeAI/agents-sdk.git
   ```

2. Navigate to SDK:
   ```bash
   cd agents-sdk
   ```

3. Obtain API keys:
   - For OpenAI models: Get an API key from [OpenAI's platform](https://platform.openai.com/api-keys)
   - For Anthropic models: Get an API key from [Anthropic's console](https://console.anthropic.com/account/keys)
   - For Google models: Get an API key from [Google AI Studio](https://ai.google.dev/)
   - For Websearch tool: Get an API key from [serpapi](https://serpapi.com)

### Building

Build everything in this space:

```bash
bazel build ...
```

### Configuration

You can configure API keys and other settings in three ways:

1. Using a `.env` file:
   ```bash
   # Copy the template
   cp .env.template .env

   # Edit the file with your API keys
   vi .env  # or use any editor
   ```

2. Using environment variables:
   ```bash
   export OPENAI_API_KEY=your_api_key_here
   export ANTHROPIC_API_KEY=your_api_key_here
   export WEBSEARCH_API_KEY=your_api_key_here
   ```

3. Passing API keys as command-line arguments (not recommended for production):
   ```bash
   bazel run examples:simple_agent -- your_api_key_here
   ```

The framework will check for API keys in the following order:
1. `.env` file
2. Environment variables
3. Command-line arguments

#### Python Tool Setup
In order to use the Python Code Execution Tool, ensure your Python environment is correctly configured so that the SDK can locate your Python runtime and libraries.
```bash
export PYTHONHOME=$(python3 -c "import sys; print(sys.prefix)")
export PYTHONPATH=$(python3 -c "import sysconfig; print(sysconfig.get_path('stdlib'))")
```

### Usage

Here's a simple example of creating and running an autonomous agent:

```cpp
#include <agents-cpp/context.h>
#include <agents-cpp/agents/autonomous_agent.h>
#include <agents-cpp/llm_interface.h>
#include <agents-cpp/tools/tool_registry.h>

using namespace agents;

int main() {
    // Create LLM
    auto llm = createLLM("anthropic", "<your_api_key_here>", "claude-3-5-sonnet-20240620");

    // Create agent context
    auto context = std::make_shared<Context>();
    context->setLLM(llm);

    // Register tools
    context->registerTool(tools::createWebSearchTool());

    // Create the agent
    AutonomousAgent agent(context);
    agent.setPlanningStrategy(AutonomousAgent::PlanningStrategy::REACT);

    // Run the agent
    JsonObject result = agent.run("Research the latest developments in quantum computing");

    // Access the result
    std::cout << result["answer"].get<std::string>() << std::endl;

    return 0;
}
```

### Running Your First Example

The simplest way to start is with the `simple_agent` example, which creates a basic autonomous agent that can use tools to answer questions:

1. Navigate to the release directory:
   ```bash
   cd agents-sdk
   ```

1. From the release directory, run the example:
   ```bash
   bazel run examples:simple_agent -- your_api_key_here
   ```

   Alternatively, you can set your API key as an environment variable:
   ```bash
   export OPENAI_API_KEY=your_api_key_here
   bazel run examples:simple_agent your_api_key_here
   ```

2. Once running, you'll be prompted to enter a question or task. For example:
   ```
   Enter a question or task for the agent (or 'exit' to quit):
   > What's the current status of quantum computing research?
   ```

3. The agent will:
   - Break down the task into steps
   - Use tools (like web search) to gather information
   - Ask for your approval before proceeding with certain steps (if human-in-the-loop is enabled)
   - Provide a comprehensive answer

4. Example output:
   ```
   Step: Planning how to approach the question
   Status: Completed
   Result: {
     "plan": "1. Search for recent quantum computing research developments..."
   }
   --------------------------------------
   Step: Searching for information on quantum computing research
   Status: Waiting for approval
   Context: {"search_query": "current status quantum computing research 2024"}
   Approve this step? (y/n): y
   ...
   ```

### Configuring the Example

You can modify examples/simple_agent.cpp to explore different configurations:

- Change the LLM provider:
  ```cpp
  // For Anthropic Claude
  auto llm = createLLM("anthropic", api_key, "claude-3-5-sonnet-20240620");

  // For Google Gemini
  auto llm = createLLM("google", api_key, "gemini-pro");
  ```

- Add different tools:
  ```cpp
  // Add more built-in tools
  context->registerTool(tools::createCalculatorTool());
  context->registerTool(tools::createPythonCodeExecutionTool());
  ```

- Change the planning strategy:
  ```cpp
  // Use ReAct planning (reasoning + acting)
  agent.setPlanningStrategy(AutonomousAgent::PlanningStrategy::REACT);

  // Or use CoT planning (chain-of-thought)
  agent.setPlanningStrategy(AutonomousAgent::PlanningStrategy::COT);
  ```

## 🧪 Included Examples

The repository includes several examples demonstrating different workflow patterns:

| Example                       | Description                           |
| ----------------------------- | --------------------------------------|
| `simple_agent`                | Basic autonomous agent                |
| `prompt_chain_example`        | Prompt chaining workflow              |
| `routing_example`             | Multi-agent routing                   |
| `parallel_example`            | Parallel task execution               |
| `orchestrator_example`        | Orchestrator–worker pattern           |
| `evaluator_optimizer_example` | Evaluator–optimizer feedback loop     |
| `multimodal_example`          | Support for voice, audio, image, docs |
| `autonomous_agent_example`    | Full-featured autonomous agent        |

Run examples available:

```bash
bazel run examples:<simple_agent> -- your_api_key_here
```

## 📂 Project Structure

- `lib/`: Public library for SDK
- `include/agents-cpp/`: Public headers
  - `types.h`: Common type definitions
  - `context.h`: Context for agent execution
  - `llm_interface.h`: Interface for LLM providers
  - `tool.h`: Tool interface
  - `memory.h`: Agent memory interface
  - `workflow.h`: Base workflow interface
  - `agent.h`: Base agent interface
  - `workflows/`: Workflow pattern implementations
  - `agents/`: Agent implementations
  - `tools/`: Tool implementations
  - `llms/`: LLM provider implementations
- `bin/examples/`: Example applications

## 🛠️ Extending the SDK

### Adding Custom Tools

```cpp
auto custom_tool = createTool(
    "calculator",
    "Evaluates mathematical expressions",
    {
        {"expression", "The expression to evaluate", "string", true}
    },
    [](const JsonObject& params) -> ToolResult {
        std::string expr = params["expression"];
        // Implement calculation logic here
        double result = evaluate(expr);
        return ToolResult{
            true,
            "Result: " + std::to_string(result),
            {{"result", result}}
        };
    }
);

context->registerTool(custom_tool);
```

### Creating Custom Workflows

You can create custom workflows by extending the `Workflow` base class or combining existing workflows:

```cpp
class CustomWorkflow : public Workflow {
public:
    CustomWorkflow(std::shared_ptr<Context> context)
        : Workflow(context) {}

    JsonObject run(const std::string& input) override {
        // Implement your custom workflow logic here
    }
};
```

## 🆘 Support

- Email: support@runedge.ai
- Discord: https://discord.gg/D5unWmt8

## 📚 Acknowledgements

This implementation is inspired by Anthropic's article ["Building effective agents"](https://www.anthropic.com/research/building-effective-agents) and and re-engineered in C++ for real-time, usage on edge devices.

## ⚖️ License

This project is licensed under a proprietary License - see the [LICENSE](./LICENSE.md) file for details.

---

<p align="center">
  <strong>The future of AI is on-device</strong><br>
  Start with our samples and discover how we could empower the next generation of AI applications.
</p>
