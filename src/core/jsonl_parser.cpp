#include "core/jsonl_parser.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace ccsm {

using json = nlohmann::json;

static constexpr int MAX_SCAN_FORWARD_LINES = 10;
static constexpr int MAX_USER_EVENT_SCAN = 20;

std::optional<SessionMeta> extract_session_meta(const std::filesystem::path& jsonl_path) {
    std::ifstream file(jsonl_path);
    if (!file.is_open()) return std::nullopt;

    std::string line;
    int lines_read = 0;

    while (std::getline(file, line) && lines_read < MAX_SCAN_FORWARD_LINES) {
        ++lines_read;
        if (line.empty()) continue;

        try {
            auto j = json::parse(line);
            auto sid = j.value("sessionId", "");
            if (!sid.empty()) {
                SessionMeta meta;
                meta.session_id = sid;
                meta.cwd = j.value("cwd", "");
                meta.git_branch = j.value("gitBranch", "");
                meta.version = j.value("version", "");
                meta.is_sidechain = j.value("isSidechain", false);
                return meta;
            }
        } catch (...) {
            continue;
        }
    }

    return std::nullopt;
}

ToolUsage extract_tool_usage(const std::filesystem::path& jsonl_path) {
    ToolUsage usage;

    std::ifstream file(jsonl_path);
    if (!file.is_open()) return usage;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        try {
            auto j = json::parse(line);
            if (j.value("type", "") != "assistant") continue;

            if (!j.contains("message")) continue;
            const auto& msg = j["message"];
            if (!msg.contains("content") || !msg["content"].is_array()) continue;

            for (const auto& block : msg["content"]) {
                if (block.value("type", "") != "tool_use") continue;

                auto name = block.value("name", "");
                usage.tool_counts[name]++;

                if (!block.contains("input")) continue;
                const auto& input = block["input"];

                if (name == "Edit" && input.contains("file_path")) {
                    usage.edited_files.push_back(input["file_path"].get<std::string>());
                } else if (name == "Write" && input.contains("file_path")) {
                    usage.created_files.push_back(input["file_path"].get<std::string>());
                } else if (name == "Read" && input.contains("file_path")) {
                    usage.read_files.push_back(input["file_path"].get<std::string>());
                }
            }
        } catch (...) {
            continue;
        }
    }

    return usage;
}

std::optional<std::string> extract_first_user_prompt(const std::filesystem::path& jsonl_path) {
    std::ifstream file(jsonl_path);
    if (!file.is_open()) return std::nullopt;

    std::string line;
    int user_events_seen = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        try {
            auto j = json::parse(line);
            if (j.value("type", "") != "user") continue;

            ++user_events_seen;
            if (user_events_seen > MAX_USER_EVENT_SCAN) break;

            if (!j.contains("message")) continue;
            const auto& msg = j["message"];
            if (msg.contains("content") && msg["content"].is_string()) {
                auto content = msg["content"].get<std::string>();
                if (!content.empty()) return content;
            }
        } catch (...) {
            continue;
        }
    }

    return std::nullopt;
}

} // namespace ccsm
