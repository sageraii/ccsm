#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <filesystem>

namespace ccsm {

enum class SessionStatus {
    active,   // JSONL file or session directory exists
    expired   // only in history.jsonl
};

struct Session {
    std::string session_id;
    std::string project_path;
    std::string first_prompt;
    std::optional<std::string> summary;
    std::optional<int> message_count;
    std::optional<std::string> git_branch;
    std::optional<std::string> cwd;
    SessionStatus status = SessionStatus::expired;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
    bool is_sidechain = false;

    // JSONL file path (set by scanner if file exists)
    std::optional<std::filesystem::path> jsonl_path;

    // Subagent metadata (set by scanner)
    int subagent_count = 0;
    std::vector<std::string> subagent_types;

    // User metadata (from tags.json)
    std::vector<std::string> tags;
    std::optional<std::string> note;
    bool is_favorite = false;

    // Summary display with fallback chain:
    // Level 1: summary → Level 2: first_prompt → Level 3: JSONL scan (added later) → Level 4: "(내용 없음)"
    [[nodiscard]] std::string display_summary() const {
        // Level 1
        if (summary.has_value() && !summary->empty()) {
            return *summary;
        }
        // Level 2
        if (!first_prompt.empty()) {
            return first_prompt;
        }
        // Level 3 will be added after JSONL parser is implemented
        // Level 4
        return "(내용 없음)";
    }
};

} // namespace ccsm
