#pragma once

#include "core/session.hpp"
#include "core/session_store.hpp"
#include "core/tag_manager.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace ccsm {

class TUIApp {
public:
    TUIApp(SessionStore& store, TagManager& tags);
    int run();

private:
    SessionStore& store_;
    TagManager& tags_;

    std::string search_text_;
    bool show_expired_ = false;
    int selected_index_ = 0;
    std::vector<Session> visible_sessions_;

    // Input mode for tag/note prompts
    enum class InputMode { none, tag, note };
    InputMode input_mode_ = InputMode::none;
    std::string input_text_;

    // Info overlay
    bool show_info_ = false;

    void refresh_visible();
    ftxui::Element render_session(const Session& s, bool selected);
    ftxui::Element render_info_overlay();
    void handle_resume();
    void handle_tag();
    void handle_note();
    void handle_favorite();

    // Helpers
    static std::string format_time(const std::chrono::system_clock::time_point& tp);
    static std::string format_status(SessionStatus status);
    static std::string truncate(const std::string& s, std::size_t max_len);
    static std::string project_name(const std::string& project_path);
};

} // namespace ccsm
