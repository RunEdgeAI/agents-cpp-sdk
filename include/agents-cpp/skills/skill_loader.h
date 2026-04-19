/**
 * @file skill_loader.h
 * @brief Skill loader definition
 * @version 0.1
 * @date 2026-03-28
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 */
#pragma once

#include <agents-cpp/tool.h>
#include <agents-cpp/types.h>

#include <memory>
#include <string>
#include <vector>

namespace agents {
/**
 * @brief Agent Skills Namespace
 */
namespace skills {

/**
 * @brief Represents a skill loaded from a markdown file
 */
struct SkillMetadata {
    /**
     * @brief Name of skill
     */
    std::string name;
    /**
     * @brief Description of skill
     */
    std::string description;
    /**
     * @brief List of binaries required
     */
    std::vector<std::string> requires_bins;
    /**
     * @brief List of environment variables required
     */
    std::vector<std::string> requires_env;
    /**
     * @brief Instruction provided by this skill
     */
    std::string instruction;
};

/**
 * @brief Tool implementation for a Markdown-based skill
 */
class MarkdownSkillTool : public Tool {
public:
    /**
     * @brief Construct a new Markdown Skill Tool object
     * @param metadata The skill metadata
     */
    explicit MarkdownSkillTool(const SkillMetadata& metadata);

    /**
     * @brief Execute the tool
     * @param params Execution parameters
     * @return ToolResult The execution result
     */
    ToolResult execute(const JsonObject& params) const override;

    /**
     * @brief Get the skill metadata
     * @return const SkillMetadata&
     */
    const SkillMetadata& getMetadata() const;

    /**
     * @brief Score the relevance of this skill to a given query
     * @param query The query to score against
     * @return double Relevance score (0.0 to 1.0)
     */
    double scoreRelevance(const std::string& query) const override;

private:
    SkillMetadata metadata_;
};

/**
 * @brief Loader for finding and parsing Markdown skills
 */
class SkillLoader {
public:
    /**
     * @brief Load all skills from the given directories
     * @param directories List of directories to search for .md files
     * @return std::vector<std::shared_ptr<MarkdownSkillTool>> Loaded skills
     */
    static std::vector<std::shared_ptr<MarkdownSkillTool>> loadFromDirectories(const std::vector<std::string>& directories);

    /**
     * @brief Load skills from standard default paths:
     *   ~/.agents/skills/, ~/.claude/skills/, <cwd>/skills/, <cwd>/.agents/skills/
     * @return std::vector<std::shared_ptr<MarkdownSkillTool>> Loaded skills
     */
    static std::vector<std::shared_ptr<MarkdownSkillTool>> loadFromDefaultPaths();

    /**
     * @brief Parse a single skill file
     * @param filepath Path to the markdown file
     * @param metadata Output metadata object
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseSkillFile(const std::string& filepath, SkillMetadata& metadata);

    /**
     * @brief Check if a skill's requirements are met
     * @param skill The skill to check
     * @return true if all required binaries are available
     */
    static bool checkRequirements(const MarkdownSkillTool& skill);
};

} // namespace skills
} // namespace agents