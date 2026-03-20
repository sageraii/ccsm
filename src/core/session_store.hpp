#pragma once

#include "core/session.hpp"
#include <vector>
#include <string_view>
#include <chrono>

namespace ccsm {

class TagManager; // forward declaration

class SessionStore {
public:
    explicit SessionStore(std::vector<Session> sessions);

    const std::vector<Session>& all() const { return sessions_; }
    std::size_t size() const { return sessions_.size(); }

    std::vector<Session> find_by_prefix(std::string_view prefix) const;

    std::vector<Session> filter_active() const;
    std::vector<Session> filter_project(std::string_view project_query) const;
    std::vector<Session> filter_branch(std::string_view branch) const;
    std::vector<Session> filter_since(std::chrono::seconds since) const;
    std::vector<Session> filter_tag(const std::string& tag, const TagManager& tags) const;

    static std::vector<Session> sort_by_date(std::vector<Session> sessions);
    static std::vector<Session> sort_by_messages(std::vector<Session> sessions);
    static std::vector<Session> sort_by_project(std::vector<Session> sessions);

    std::vector<Session> search(std::string_view keyword) const;

private:
    std::vector<Session> sessions_;
};

} // namespace ccsm
