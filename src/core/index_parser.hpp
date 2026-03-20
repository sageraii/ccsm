#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace ccsm {

struct IndexEntry {
    std::string session_id;
    std::string full_path;       // NOTE: typically dead link, do not rely on
    std::string first_prompt;
    std::optional<std::string> summary;
    int message_count = 0;
    std::string created;
    std::string modified;
    std::string git_branch;
    std::string project_path;
    bool is_sidechain = false;
};

struct IndexFile {
    int version = 0;
    std::optional<std::string> original_path;
    std::vector<IndexEntry> entries;
};

IndexFile parse_index(const std::filesystem::path& index_path);

} // namespace ccsm
