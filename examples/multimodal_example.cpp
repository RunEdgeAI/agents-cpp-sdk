/**
 * @example multimodal_example.cpp
 * @brief Multimodal understanding example with image, audio, and video
 * @version 0.1
 * @date 2025-09-21
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */

#include <agents-cpp/config_loader.h>
#include <agents-cpp/context.h>
#include <agents-cpp/logger.h>
#include <agents-cpp/tools/tool_registry.h>

#include <iostream>

using namespace agents;

Task<int> runmultimodalExample(std::string media_path) {
    // Initialize logger
    Logger::init(Logger::Level::INFO);

    auto& config = ConfigLoader::getInstance();

    // Choose provider here
    std::string provider = "google";
    std::string model = "gemini-2.5-flash-lite";
    std::string api_key = config.get("GEMINI_API_KEY");

    if (api_key.empty()) {
        std::cerr << "GEMINI_API_KEY not set.\n";
        co_return EXIT_FAILURE;
    }

    // Create the context
    auto context = std::make_shared<Context>();

    // Create the LLM
    auto llm = createLLM(provider, api_key, model);

    // Configure LLM options
    LLMOptions options;
    options.temperature = 0.7;
    llm->setOptions(options);

    // Set the LLM
    context->setLLM(llm);

    // Register tools
    context->registerTool(tools::createMediaLoaderTool(llm));

    // Set the system prompt
    context->setSystemPrompt(
        "You are a friendly assistant that helps users find information and answer questions. "
        "Use the tools available to you to load files, gather information, and provide comprehensive answers. "
        "Always keep your answers concise and in less than 50 words. "
    );

    // Lambda for printing result stream to console
    auto printStream = [&](AsyncGenerator<std::string>& generator) -> Task<void> {
        // Display the result as it arrives
        while (auto item = co_await generator.next()) {
            std::string chunk = *item;
            std::cout << chunk << std::flush;
        }
        std::cout << std::endl << std::endl;
    };

    // Examples demonstrating chat with media URI; prefer https:// or file:// for large images
    // Alternatively, provide the base64 encoded media directly using:
    // data:<mime_type>;base64,<data>
    // For example: "data:image/png;base64,AAAA..."
    try {
        // Example 0: Multimodal Chat With Document
        auto doc_resp = context->streamChat(
            "Describe what is in this document.",
            { "file://" + media_path + "/docs/resume.pdf" }
        );
        co_await printStream(doc_resp);

        // Example 1: Multimodal Chat With Audio
        auto audio_resp = context->streamChat(
            "Describe what is in this audio.",
            { "file://" + media_path + "/audio/sample.mp3" }
        );
        co_await printStream(audio_resp);

        // Example 2: Multimodal Chat With Video
        auto video_resp = context->streamChat(
            "Describe what is this video about.",
            { "file://" + media_path + "/video/sample_video.mp4" }
        );
        co_await printStream(video_resp);

        // Example 3: Multimodal Chat With Multiple Images
        auto image_resp = context->streamChat(
            "Describe what is happening in these images.",
            {
                "https://i.ytimg.com/vi/Eb4ICVPOUlI/hqdefault.jpg",
                "file://" + media_path + "/scenes/robotics_scene.png",
                "data:image/png;base64,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"
            }
        );
        co_await printStream(image_resp);

        // Optionally ask quesions about the content
        std::string user_input;
        while (true) {
            Logger::info("Enter questions about the content (type 'exit', 'quit' or 'q' to stop):");
            Logger::info("> ");
            std::getline(std::cin, user_input);

            if (user_input == "exit" || user_input == "quit" || user_input == "q") {
                break;
            }

            if (user_input.empty()) {
                continue;
            }

            // Chat with the model
            auto chat_response = context->streamChat(user_input);
            co_await printStream(chat_response);
        }
    } catch (const std::exception& e) {
        Logger::error("Error: {}", e.what());
    }

    co_return EXIT_SUCCESS;
}

int main(int argc, char**argv) {
    if (argc != 2)
    {
        Logger::error("Usage: ./multimodal_example <absolute_path_to_media_dir>");
        return EXIT_FAILURE;
    }
    return blockingWait(runmultimodalExample(std::string(argv[1])));
}
