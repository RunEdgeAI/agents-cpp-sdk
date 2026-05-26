/**
 * @example robotics_object_detection_demo.cpp
 * @brief Robotics object detection demo
 * @version 0.1
 * @date 2025-01-27
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */

#include <agents-cpp/context.h>
#include <agents-cpp/config_loader.h>
#include <agents-cpp/llm_interface.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

using namespace agents;

Task<int> runRoboticsObjectDetectionDemo(std::string media_path) {
    // Initialize logger
    Logger::init(Logger::Level::INFO);

    auto& config = ConfigLoader::getInstance();

    // Use Google Gemini for robotics capabilities
    std::string provider = "google";
    std::string model = "gemini-2.5-flash"; // Can also use gemini-robotics-er-1.5-preview when available
    std::string api_key = config.get("GEMINI_API_KEY");

    if (api_key.empty()) {
        Logger::error("GEMINI_API_KEY not set.");
        co_return EXIT_FAILURE;
    }

    // Create the context
    auto context = std::make_shared<Context>();

    // Create the LLM
    auto llm = createLLM(provider, api_key, model);

    // Configure LLM options for robotics tasks
    LLMOptions options;
    options.temperature = 0.5; // Lower temperature for more consistent results
    options.max_tokens = 4096;
    llm->setOptions(options);

    // Set the LLM
    context->setLLM(llm);

    // Register tools
    context->registerTool(tools::createMediaLoaderTool(llm));

    // Set the system prompt for robotics object detection
    context->setSystemPrompt(
        "You are a robotics vision system specialized in object detection and spatial reasoning. "
        "You can identify objects in images and provide precise 2D coordinates for robotic manipulation. "
        "Always respond with valid JSON format as requested."
    );

    try {
        // Demo 1: Pointing to objects (using Gemini Robotics-ER 1.5)
        Logger::info("=== Robotics Object Detection Demo ===");
        Logger::info("Detecting objects and providing 2D coordinates...");

        std::string robotics_prompt = R"(
Point to no more than 10 items in the image. The label returned
should be an identifying name for the object detected.
The answer should follow the json format: [{"point": [y, x], "label": "<label1>"}, ...].
The points are in [y, x] format normalized to 0-1000.
        )";

        // Use a sample image - replace with your robotics scene image
        auto object_detection_resp = co_await context->chatWithTools(
            robotics_prompt,
            { "file://" + media_path + "/scenes/synthetic_table.png" }
        );

        Logger::info("Object Detection Results:");
        Logger::info("{}", object_detection_resp.content);

        // Demo 2: Object Finding with Natural Language
        Logger::info("\n=== Object Finding Demo ===");
        Logger::info("Finding specific objects based on natural language commands...");

        std::string find_prompt = R"(
Find the banana in the image. Return the coordinates and label in JSON format:
[{"point": [y, x], "label": "banana"}]
        )";

        auto find_resp = co_await context->chatWithTools(
            find_prompt,
            { "file://" + media_path + "/scenes/synthetic_table.png" }
        );

        Logger::info("Object Finding Results:");
        Logger::info("{}", find_resp.content);

        // Demo 3: Object Detection & Bounding Boxes
        Logger::info("\n=== Object Detection & Bounding Boxes Demo ===");
        Logger::info("Analyzing scene for objects and their bounding boxes...");

        std::string bounding_boxes_prompt = R"(
Analyze objects in this scene and return bounding boxes as a JSON array with labels.
Never return masks or code fencing. Limit to 25 objects. Include as many objects as you
can identify on the table.
If an object is present multiple times, name them according to their
unique characteristic (colors, size, position, unique characteristics, etc..).
The format should be as follows: [{"box_2d": [ymin, xmin, ymax, xmax],
"label": <label for the object>}] normalized to 0-1000. The values in
box_2d must only be integers
        )";

        auto bounding_boxes_resp = co_await context->chatWithTools(
            bounding_boxes_prompt,
            { "file://" + media_path + "/scenes/synthetic_table.png" }
        );

        Logger::info("Bounding Boxes Results:");
        Logger::info("{}", bounding_boxes_resp.content);

        // Demo 4: Scene Understanding for Robotics
        Logger::info("\n=== Scene Understanding Demo ===");
        Logger::info("Analyzing scene for robotic manipulation...");

        std::string scene_prompt = R"(
Analyze this scene for robotic manipulation. Identify:
1. A few graspable objects with their coordinates
2. Potential obstacles or hazards
3. Objects that can be picked up together
4. Objects that require careful handling

Return results in JSON format with categories and coordinates.
        )";

        auto scene_resp = co_await context->chatWithTools(
            scene_prompt,
            { "file://" + media_path + "/scenes/synthetic_table.png" }
        );

        Logger::info("Scene Analysis Results:");
        Logger::info("{}", scene_resp.content);

    } catch (const std::exception& e) {
        Logger::error("Error in robotics demo: {}", e.what());
        co_return EXIT_FAILURE;
    }

    co_return EXIT_SUCCESS;
}

int main(int argc, char**argv) {
    if (argc != 2)
    {
        Logger::error("Usage: ./robotics_object_detection_demo <absolute_path_to_media_dir>");
        return EXIT_FAILURE;
    }
    return blockingWait(runRoboticsObjectDetectionDemo(std::string(argv[1])));
}
