#pragma once

#include "core/session.hpp"
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace ccsm {

struct ScanConfig {
    std::filesystem::path history_path;
    std::vector<std::filesystem::path> project_dirs;
    std::filesystem::path claude_dir;
};

class SessionScanner {
public:
    explicit SessionScanner(const ScanConfig& config);

    std::vector<Session> scan();
    static ScanConfig default_config();

private:
    ScanConfig config_;

    void stage1_history(std::unordered_map<std::string, Session>& sessions);
    void stage2_index(std::unordered_map<std::string, Session>& sessions);
    void stage3_filesystem(std::unordered_map<std::string, Session>& sessions);

    void merge_index_into(Session& session, const struct IndexEntry& entry);
    void scan_session_directory(const std::filesystem::path& dir, Session& session);
    static bool is_uuid(const std::string& name);
};

} // namespace ccsm
