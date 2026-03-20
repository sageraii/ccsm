#include "core/tag_manager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ccsm {

using json = nlohmann::json;

TagManager::TagManager(const std::filesystem::path& tags_path) : path_(tags_path) {
    load();
}

void TagManager::load() {
    std::ifstream file(path_);
    if (!file.is_open()) return;

    try {
        auto j = json::parse(file);
        if (!j.is_object() || !j.contains("sessions") || !j["sessions"].is_object()) return;
        for (auto& [sid, val] : j["sessions"].items()) {
            TagData td;
            if (val.contains("tags") && val["tags"].is_array()) {
                for (const auto& t : val["tags"]) {
                    td.tags.push_back(t.get<std::string>());
                }
            }
            td.note = val.value("note", "");
            td.favorite = val.value("favorite", false);
            td.last_seen = val.value("lastSeen", "");
            data_[sid] = std::move(td);
        }
    } catch (...) {}
}

std::optional<TagData> TagManager::get(const std::string& session_id) const {
    auto it = data_.find(session_id);
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

TagData& TagManager::ensure_entry(const std::string& session_id) {
    return data_[session_id];
}

void TagManager::add_tag(const std::string& session_id, const std::string& tag) {
    auto& entry = ensure_entry(session_id);
    if (std::find(entry.tags.begin(), entry.tags.end(), tag) == entry.tags.end()) {
        entry.tags.push_back(tag);
    }
}

void TagManager::remove_tag(const std::string& session_id, const std::string& tag) {
    auto it = data_.find(session_id);
    if (it == data_.end()) return;
    auto& tags = it->second.tags;
    tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

void TagManager::set_note(const std::string& session_id, const std::string& note) {
    ensure_entry(session_id).note = note;
}

void TagManager::toggle_favorite(const std::string& session_id) {
    auto& entry = ensure_entry(session_id);
    entry.favorite = !entry.favorite;
}

void TagManager::update_last_seen(const std::string& session_id) {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%FT%TZ", std::gmtime(&t));
    ensure_entry(session_id).last_seen = buf;
}

std::vector<std::string> TagManager::get_favorites() const {
    std::vector<std::string> result;
    for (const auto& [sid, td] : data_) {
        if (td.favorite) result.push_back(sid);
    }
    return result;
}

std::vector<std::string> TagManager::find_by_tag(const std::string& tag) const {
    std::vector<std::string> result;
    for (const auto& [sid, td] : data_) {
        if (std::find(td.tags.begin(), td.tags.end(), tag) != td.tags.end()) {
            result.push_back(sid);
        }
    }
    return result;
}

void TagManager::save() const {
    namespace fs = std::filesystem;

    if (path_.has_parent_path()) {
        fs::create_directories(path_.parent_path());
    }

    auto tmp_path = path_;
    tmp_path += ".tmp";

    json j;
    j["version"] = 1;
    json sessions = json::object();

    for (const auto& [sid, td] : data_) {
        json entry;
        entry["tags"] = td.tags;
        entry["note"] = td.note;
        entry["favorite"] = td.favorite;
        entry["lastSeen"] = td.last_seen;
        sessions[sid] = entry;
    }
    j["sessions"] = sessions;

    std::ofstream file(tmp_path);
    file << j.dump(2);
    file.close();

    fs::rename(tmp_path, path_);
}

void TagManager::cleanup(const std::vector<std::string>& active_session_ids, int max_age_days) {
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_remove;

    for (const auto& [sid, td] : data_) {
        // Keep active sessions
        if (std::find(active_session_ids.begin(), active_session_ids.end(), sid)
            != active_session_ids.end()) {
            continue;
        }

        // Check age — if lastSeen is within max_age_days, keep it
        if (!td.last_seen.empty()) {
            std::tm tm = {};
            std::istringstream ss(td.last_seen);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            if (!ss.fail()) {
                // Use timegm for UTC (lastSeen has Z suffix)
                auto last = std::chrono::system_clock::from_time_t(timegm(&tm));
                auto age = std::chrono::duration_cast<std::chrono::hours>(now - last);
                if (age.count() < max_age_days * 24) {
                    continue; // within age limit, keep
                }
            }
        }

        to_remove.push_back(sid);
    }

    for (const auto& sid : to_remove) {
        data_.erase(sid);
    }
}

} // namespace ccsm
