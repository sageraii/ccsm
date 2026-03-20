#include "core/session_store.hpp"
#include "core/tag_manager.hpp"
#include <algorithm>

namespace ccsm {

SessionStore::SessionStore(std::vector<Session> sessions)
    : sessions_(std::move(sessions)) {}

std::vector<Session> SessionStore::find_by_prefix(std::string_view prefix) const {
    std::vector<Session> result;
    for (const auto& s : sessions_) {
        if (s.session_id.size() >= prefix.size() &&
            s.session_id.substr(0, prefix.size()) == prefix) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<Session> SessionStore::filter_active() const {
    std::vector<Session> result;
    for (const auto& s : sessions_) {
        if (s.status == SessionStatus::active) result.push_back(s);
    }
    return result;
}

std::vector<Session> SessionStore::filter_project(std::string_view query) const {
    std::vector<Session> result;
    for (const auto& s : sessions_) {
        if (s.project_path.find(query) != std::string::npos) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<Session> SessionStore::filter_branch(std::string_view branch) const {
    std::vector<Session> result;
    for (const auto& s : sessions_) {
        if (s.git_branch.has_value() && *s.git_branch == branch) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<Session> SessionStore::filter_since(std::chrono::seconds since) const {
    auto cutoff = std::chrono::system_clock::now() - since;
    std::vector<Session> result;
    for (const auto& s : sessions_) {
        if (s.modified_at >= cutoff) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<Session> SessionStore::filter_tag(const std::string& tag, const TagManager& tags) const {
    auto tagged_ids = tags.find_by_tag(tag);
    std::vector<Session> result;
    for (const auto& s : sessions_) {
        if (std::find(tagged_ids.begin(), tagged_ids.end(), s.session_id) != tagged_ids.end()) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<Session> SessionStore::sort_by_date(std::vector<Session> sessions) {
    std::sort(sessions.begin(), sessions.end(),
        [](const Session& a, const Session& b) { return a.modified_at > b.modified_at; });
    return sessions;
}

std::vector<Session> SessionStore::sort_by_messages(std::vector<Session> sessions) {
    std::sort(sessions.begin(), sessions.end(),
        [](const Session& a, const Session& b) {
            return a.message_count.value_or(0) > b.message_count.value_or(0);
        });
    return sessions;
}

std::vector<Session> SessionStore::sort_by_project(std::vector<Session> sessions) {
    std::sort(sessions.begin(), sessions.end(),
        [](const Session& a, const Session& b) { return a.project_path < b.project_path; });
    return sessions;
}

std::vector<Session> SessionStore::search(std::string_view keyword) const {
    std::vector<Session> result;
    std::string kw_lower(keyword);
    std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);

    for (const auto& s : sessions_) {
        auto check = [&](const std::string& field) {
            std::string lower = field;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            return lower.find(kw_lower) != std::string::npos;
        };

        bool found = check(s.first_prompt) ||
            (s.summary.has_value() && check(*s.summary)) ||
            check(s.project_path) ||
            check(s.session_id);

        // Search tags
        if (!found) {
            for (const auto& tag : s.tags) {
                if (check(tag)) { found = true; break; }
            }
        }

        // Search note
        if (!found && s.note.has_value()) {
            found = check(*s.note);
        }

        if (found) result.push_back(s);
    }
    return result;
}

} // namespace ccsm
