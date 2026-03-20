#include "core/session_scanner.hpp"
#include "core/history_parser.hpp"
#include "core/index_parser.hpp"
#include "core/jsonl_parser.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <regex>

namespace ccsm {

using json = nlohmann::json;

static const std::regex uuid_regex(
    "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}",
    std::regex::icase
);

SessionScanner::SessionScanner(const ScanConfig& config)
    : config_(config) {}

bool SessionScanner::is_uuid(const std::string& name) {
    return std::regex_match(name, uuid_regex);
}

std::vector<Session> SessionScanner::scan() {
    std::unordered_map<std::string, Session> sessions;

    stage1_history(sessions);
    stage2_index(sessions);
    stage3_filesystem(sessions);

    std::vector<Session> result;
    result.reserve(sessions.size());
    for (auto& [id, session] : sessions) {
        result.push_back(std::move(session));
    }
    return result;
}

ScanConfig SessionScanner::default_config() {
    auto home = get_home_dir();
    auto claude_dir = home / ".claude";

    ScanConfig config;
    config.claude_dir = claude_dir;
    config.history_path = claude_dir / "history.jsonl";

    // Discover project dirs: ~/.claude/projects/<encoded-path>/
    auto projects_dir = claude_dir / "projects";
    if (std::filesystem::exists(projects_dir) && std::filesystem::is_directory(projects_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(projects_dir)) {
            if (entry.is_directory()) {
                config.project_dirs.push_back(entry.path());
            }
        }
    }

    return config;
}

// Stage 1: Parse history.jsonl as the primary source
void SessionScanner::stage1_history(std::unordered_map<std::string, Session>& sessions) {
    if (!std::filesystem::exists(config_.history_path)) return;

    auto history = parse_history(config_.history_path);
    for (auto& s : history) {
        sessions[s.session_id] = std::move(s);
    }
}

// Stage 2: Merge sessions-index.json data from each project dir
void SessionScanner::stage2_index(std::unordered_map<std::string, Session>& sessions) {
    for (const auto& dir : config_.project_dirs) {
        auto index_path = dir / "sessions-index.json";
        if (!std::filesystem::exists(index_path)) continue;

        auto index_file = parse_index(index_path);
        for (const auto& entry : index_file.entries) {
            auto it = sessions.find(entry.session_id);
            if (it != sessions.end()) {
                merge_index_into(it->second, entry);
            } else {
                // New session from index only
                Session s;
                s.session_id = entry.session_id;
                s.project_path = entry.project_path;
                s.first_prompt = entry.first_prompt;
                s.is_sidechain = entry.is_sidechain;
                merge_index_into(s, entry);
                sessions[s.session_id] = std::move(s);
            }
        }
    }
}

void SessionScanner::merge_index_into(Session& session, const IndexEntry& entry) {
    // summary: index provides it, history does not have it
    if (entry.summary.has_value() && !entry.summary->empty()) {
        if (!session.summary.has_value()) {
            session.summary = entry.summary;
        }
    }

    // messageCount
    if (entry.message_count > 0) {
        if (!session.message_count.has_value()) {
            session.message_count = entry.message_count;
        }
    }

    // gitBranch: only if session doesn't already have one
    if (!entry.git_branch.empty()) {
        if (!session.git_branch.has_value() || session.git_branch->empty()) {
            session.git_branch = entry.git_branch;
        }
    }

    // projectPath: only if session's is empty
    if (!entry.project_path.empty() && session.project_path.empty()) {
        session.project_path = entry.project_path;
    }

    // DO NOT use fullPath (dead links)
}

// Stage 3: Filesystem scan for JSONL files and session directories
void SessionScanner::stage3_filesystem(std::unordered_map<std::string, Session>& sessions) {
    for (const auto& dir : config_.project_dirs) {
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) continue;

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                // Check for UUID.jsonl files
                auto filename = entry.path().filename().string();
                if (filename.size() > 6 && filename.substr(filename.size() - 6) == ".jsonl") {
                    auto stem = entry.path().stem().string();
                    if (is_uuid(stem)) {
                        auto it = sessions.find(stem);
                        if (it != sessions.end()) {
                            it->second.status = SessionStatus::active;
                            it->second.jsonl_path = entry.path();
                        } else {
                            // Orphan JSONL — extract metadata
                            auto meta = extract_session_meta(entry.path());
                            Session s;
                            s.session_id = stem;
                            s.status = SessionStatus::active;
                            s.jsonl_path = entry.path();
                            if (meta.has_value()) {
                                s.cwd = meta->cwd;
                                s.git_branch = meta->git_branch;
                                s.is_sidechain = meta->is_sidechain;
                            }
                            // Try to get first user prompt
                            auto prompt = extract_first_user_prompt(entry.path());
                            if (prompt.has_value()) {
                                s.first_prompt = prompt.value();
                            }
                            sessions[s.session_id] = std::move(s);
                        }
                    }
                }
            } else if (entry.is_directory()) {
                // Check for UUID-named session directories
                auto dirname = entry.path().filename().string();
                if (is_uuid(dirname)) {
                    auto it = sessions.find(dirname);
                    if (it != sessions.end()) {
                        it->second.status = SessionStatus::active;
                        scan_session_directory(entry.path(), it->second);
                    } else {
                        // Orphan directory
                        Session s;
                        s.session_id = dirname;
                        s.status = SessionStatus::active;
                        scan_session_directory(entry.path(), s);
                        sessions[s.session_id] = std::move(s);
                    }
                }
            }
        }
    }
}

void SessionScanner::scan_session_directory(const std::filesystem::path& dir, Session& session) {
    auto subagents_dir = dir / "subagents";
    if (!std::filesystem::exists(subagents_dir) || !std::filesystem::is_directory(subagents_dir)) {
        return;
    }

    int count = 0;
    std::vector<std::string> types;

    for (const auto& entry : std::filesystem::directory_iterator(subagents_dir)) {
        if (!entry.is_regular_file()) continue;

        auto filename = entry.path().filename().string();

        // Count .jsonl files excluding compact variants
        if (filename.size() > 6 && filename.substr(filename.size() - 6) == ".jsonl") {
            if (filename.find("compact") == std::string::npos) {
                ++count;
            }
        }

        // Read .meta.json files for agentType
        if (filename.size() > 10 && filename.substr(filename.size() - 10) == ".meta.json") {
            try {
                std::ifstream f(entry.path());
                auto j = json::parse(f);
                auto agent_type = j.value("agentType", "");
                if (!agent_type.empty()) {
                    types.push_back(agent_type);
                }
            } catch (...) {
                // skip malformed meta files
            }
        }
    }

    session.subagent_count = count;
    session.subagent_types = std::move(types);
}

} // namespace ccsm
