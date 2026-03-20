#pragma once

#include "core/session.hpp"
#include "core/session_store.hpp"
#include "core/tag_manager.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>

namespace ccsm {

struct CLIError : std::runtime_error {
    int exit_code;
    CLIError(const std::string& msg, int code = 1)
        : std::runtime_error(msg), exit_code(code) {}
};

class CLIApp {
public:
    CLIApp(SessionStore& store, TagManager& tags);

    int run(int argc, char* argv[]);

private:
    SessionStore& store_;
    TagManager& tags_;

    // Subcommand handlers
    void cmd_list(bool expired, bool all, const std::string& project,
                  const std::string& branch, const std::string& since,
                  const std::string& tag, const std::string& sort,
                  const std::string& format, int limit);
    void cmd_info(const std::string& prefix);
    void cmd_search(const std::string& keyword, bool deep);
    void cmd_tag(const std::string& prefix, const std::string& tag);
    void cmd_untag(const std::string& prefix, const std::string& tag);
    void cmd_note(const std::string& prefix, const std::string& text);
    void cmd_favorite(const std::string& prefix);
    void cmd_favorites();
    void cmd_resume(const std::string& prefix);
    void cmd_cleanup();

    // Resolve session by prefix (min 3 chars), handles ambiguity
    Session resolve_session(const std::string& prefix);

    // Output formatters
    void print_table(const std::vector<Session>& sessions);
    void print_json(const std::vector<Session>& sessions);
    void print_csv(const std::vector<Session>& sessions);

    // Helper: format time_point to human-readable string
    static std::string format_time(const std::chrono::system_clock::time_point& tp);
    static std::string format_status(SessionStatus status);
    static std::string truncate(const std::string& s, std::size_t max_len);
};

} // namespace ccsm
