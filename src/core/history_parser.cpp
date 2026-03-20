#include "core/history_parser.hpp"
#include "util/text_sanitizer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <cstdlib>

namespace ccsm {

using json = nlohmann::json;

static std::chrono::system_clock::time_point from_unix_ms(int64_t ms) {
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
}

std::filesystem::path get_home_dir() {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return std::filesystem::path(home);
}

std::vector<Session> parse_history(const std::filesystem::path& history_path) {
    std::vector<Session> result;

    std::ifstream file(history_path);
    if (!file.is_open()) return result;

    struct HistoryEntry {
        std::string display;
        std::string project;
        int64_t first_timestamp;
        int64_t last_timestamp;
    };
    std::unordered_map<std::string, HistoryEntry> entries;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try {
            auto j = json::parse(line);
            auto sid = j.value("sessionId", "");
            if (sid.empty()) continue;

            auto ts = j.value("timestamp", int64_t{0});
            auto it = entries.find(sid);
            if (it == entries.end()) {
                entries[sid] = {
                    j.value("display", ""),
                    j.value("project", ""),
                    ts,
                    ts
                };
            } else {
                if (ts < it->second.first_timestamp) {
                    it->second.first_timestamp = ts;
                    it->second.display = j.value("display", "");
                }
                if (ts > it->second.last_timestamp) {
                    it->second.last_timestamp = ts;
                }
            }
        } catch (...) {
            continue; // skip malformed lines
        }
    }

    result.reserve(entries.size());
    for (auto& [sid, entry] : entries) {
        Session s;
        s.session_id = sid;
        s.project_path = entry.project;
        s.first_prompt = sanitize_prompt(entry.display);
        s.created_at = from_unix_ms(entry.first_timestamp);
        s.modified_at = from_unix_ms(entry.last_timestamp);
        s.status = SessionStatus::expired; // will be updated by scanner
        result.push_back(std::move(s));
    }

    return result;
}

} // namespace ccsm
