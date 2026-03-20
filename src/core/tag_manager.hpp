#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <filesystem>

namespace ccsm {

struct TagData {
    std::vector<std::string> tags;
    std::string note;
    bool favorite = false;
    std::string last_seen;
};

class TagManager {
public:
    explicit TagManager(const std::filesystem::path& tags_path);

    std::optional<TagData> get(const std::string& session_id) const;
    void add_tag(const std::string& session_id, const std::string& tag);
    void remove_tag(const std::string& session_id, const std::string& tag);
    void set_note(const std::string& session_id, const std::string& note);
    void toggle_favorite(const std::string& session_id);
    void update_last_seen(const std::string& session_id);

    std::vector<std::string> get_favorites() const;
    std::vector<std::string> find_by_tag(const std::string& tag) const;

    void save() const;
    void cleanup(const std::vector<std::string>& active_session_ids, int max_age_days = 30);

private:
    std::filesystem::path path_;
    std::unordered_map<std::string, TagData> data_;

    void load();
    TagData& ensure_entry(const std::string& session_id);
};

} // namespace ccsm
