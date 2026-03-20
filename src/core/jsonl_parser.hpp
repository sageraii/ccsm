#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <filesystem>

namespace ccsm {

struct SessionMeta {
    std::string session_id;
    std::string cwd;
    std::string git_branch;
    std::string version;
    bool is_sidechain = false;
};

struct ToolUsage {
    std::vector<std::string> edited_files;
    std::vector<std::string> created_files;
    std::vector<std::string> read_files;
    std::unordered_map<std::string, int> tool_counts;
};

std::optional<SessionMeta> extract_session_meta(const std::filesystem::path& jsonl_path);
ToolUsage extract_tool_usage(const std::filesystem::path& jsonl_path);
std::optional<std::string> extract_first_user_prompt(const std::filesystem::path& jsonl_path);

} // namespace ccsm
