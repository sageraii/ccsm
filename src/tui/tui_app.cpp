#include "tui/tui_app.hpp"
#include "core/jsonl_parser.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace ccsm {

namespace fs = std::filesystem;
using namespace ftxui;

TUIApp::TUIApp(SessionStore& store, TagManager& tags)
    : store_(store), tags_(tags) {
    refresh_visible();
}

int TUIApp::run() {
    auto screen = ScreenInteractive::Fullscreen();

    // Search input component
    auto search_input = Input(&search_text_, "검색...");

    // Input for tag/note prompts
    auto prompt_input = Input(&input_text_, "");

    // Track whether search box is focused
    bool search_focused = false;

    auto renderer = Renderer(search_input, [&] {
        // Refresh visible sessions based on search/filter
        refresh_visible();

        // Clamp selected index
        if (!visible_sessions_.empty()) {
            if (selected_index_ < 0) selected_index_ = 0;
            if (selected_index_ >= static_cast<int>(visible_sessions_.size())) {
                selected_index_ = static_cast<int>(visible_sessions_.size()) - 1;
            }
        } else {
            selected_index_ = 0;
        }

        // Build session list elements
        Elements session_elements;
        for (int i = 0; i < static_cast<int>(visible_sessions_.size()); ++i) {
            session_elements.push_back(render_session(visible_sessions_[static_cast<std::size_t>(i)], i == selected_index_));
        }

        if (session_elements.empty()) {
            session_elements.push_back(text("  세션이 없습니다.") | dim);
        }

        // Filter/sort labels
        std::string filter_label = show_expired_ ? "All" : "Active";

        // Top bar: search + filter + sort
        auto top_bar = hbox({
            text(" 검색: ") | bold,
            search_input->Render() | flex | (search_focused ? focus : nothing),
            separator(),
            text(" 필터: " + filter_label + " ") | bold,
            separator(),
            text(" 정렬: 최신순 ") | bold,
        }) | border;

        // Session list (scrollable)
        auto session_list = vbox(std::move(session_elements)) | vscroll_indicator | yframe | flex;

        // Bottom help bar
        Element bottom_bar;
        if (input_mode_ == InputMode::tag) {
            bottom_bar = hbox({
                text(" 태그 입력: ") | bold,
                prompt_input->Render() | flex,
                text(" [Enter] 확인  [Esc] 취소 "),
            }) | border;
        } else if (input_mode_ == InputMode::note) {
            bottom_bar = hbox({
                text(" 노트 입력: ") | bold,
                prompt_input->Render() | flex,
                text(" [Enter] 확인  [Esc] 취소 "),
            }) | border;
        } else {
            bottom_bar = hbox({
                text(" [Enter]") | bold, text(" Resume  "),
                text("[t]") | bold, text(" Tag  "),
                text("[n]") | bold, text(" Note  "),
                text("[f]") | bold, text(" Fav  "),
                text("[q]") | bold, text(" Quit  "),
                text("[/]") | bold, text(" Search  "),
                text("[e]") | bold, text(" Expired  "),
                text("[i]") | bold, text(" Info"),
            }) | border;
        }

        // Info overlay
        if (show_info_) {
            auto info = render_info_overlay();
            return vbox({
                text(" ccsm - Claude Code Session Manager ") | bold | hcenter,
                top_bar,
                session_list,
                separator(),
                info | flex,
                bottom_bar,
            }) | border;
        }

        return vbox({
            text(" ccsm - Claude Code Session Manager ") | bold | hcenter,
            top_bar,
            session_list,
            bottom_bar,
        }) | border;
    });

    // Event handler wrapping the renderer
    auto component = CatchEvent(renderer, [&](Event event) -> bool {
        // Handle input mode (tag/note prompt)
        if (input_mode_ != InputMode::none) {
            if (event == Event::Escape) {
                input_mode_ = InputMode::none;
                input_text_.clear();
                return true;
            }
            if (event == Event::Return) {
                if (!input_text_.empty() && !visible_sessions_.empty()) {
                    auto& s = visible_sessions_[static_cast<std::size_t>(selected_index_)];
                    if (input_mode_ == InputMode::tag) {
                        tags_.add_tag(s.session_id, input_text_);
                        tags_.save();
                        // Update session tags in-place
                        auto td = tags_.get(s.session_id);
                        if (td.has_value()) s.tags = td->tags;
                    } else if (input_mode_ == InputMode::note) {
                        tags_.set_note(s.session_id, input_text_);
                        tags_.save();
                        s.note = input_text_;
                    }
                }
                input_mode_ = InputMode::none;
                input_text_.clear();
                return true;
            }
            // Let the prompt_input handle character input
            return prompt_input->OnEvent(event);
        }

        // Close info overlay
        if (show_info_) {
            if (event == Event::Escape || event == Event::Character('i') ||
                event == Event::Character('q')) {
                show_info_ = false;
                return true;
            }
            return true; // consume all events while info is shown
        }

        // Search focus mode: '/' to enter, Escape to leave
        if (search_focused) {
            if (event == Event::Escape || event == Event::Return) {
                search_focused = false;
                return true;
            }
            // Let search_input handle typing
            return search_input->OnEvent(event);
        }

        // Normal mode key bindings
        if (event == Event::Character('q') || event == Event::Escape) {
            screen.Exit();
            return true;
        }

        if (event == Event::Character('/')) {
            search_focused = true;
            return true;
        }

        // Navigation
        if (event == Event::Character('j') || event == Event::ArrowDown) {
            if (!visible_sessions_.empty() &&
                selected_index_ < static_cast<int>(visible_sessions_.size()) - 1) {
                selected_index_++;
            }
            return true;
        }
        if (event == Event::Character('k') || event == Event::ArrowUp) {
            if (selected_index_ > 0) {
                selected_index_--;
            }
            return true;
        }

        // Page up/down
        if (event == Event::PageDown) {
            selected_index_ = std::min(
                selected_index_ + 10,
                static_cast<int>(visible_sessions_.size()) - 1);
            if (selected_index_ < 0) selected_index_ = 0;
            return true;
        }
        if (event == Event::PageUp) {
            selected_index_ = std::max(selected_index_ - 10, 0);
            return true;
        }

        // Home/End
        if (event == Event::Home) {
            selected_index_ = 0;
            return true;
        }
        if (event == Event::End) {
            selected_index_ = std::max(0, static_cast<int>(visible_sessions_.size()) - 1);
            return true;
        }

        // Enter: resume
        if (event == Event::Return) {
            handle_resume();
            screen.Exit();
            return true;
        }

        // t: tag
        if (event == Event::Character('t')) {
            handle_tag();
            return true;
        }

        // n: note
        if (event == Event::Character('n')) {
            handle_note();
            return true;
        }

        // f: favorite
        if (event == Event::Character('f')) {
            handle_favorite();
            return true;
        }

        // i: info
        if (event == Event::Character('i')) {
            if (!visible_sessions_.empty()) {
                show_info_ = !show_info_;
            }
            return true;
        }

        // e: toggle expired
        if (event == Event::Character('e')) {
            show_expired_ = !show_expired_;
            selected_index_ = 0;
            return true;
        }

        return false;
    });

    screen.Loop(component);
    return 0;
}

void TUIApp::refresh_visible() {
    std::vector<Session> sessions;

    if (show_expired_) {
        sessions = store_.all();
    } else {
        sessions = store_.filter_active();
    }

    // Apply search filter
    if (!search_text_.empty()) {
        std::vector<Session> filtered;
        for (const auto& s : sessions) {
            // Search in session_id, project_path, first_prompt, summary, tags, note, branch
            auto matches = [&](const std::string& haystack) {
                if (haystack.empty()) return false;
                // Case-insensitive search
                std::string lower_hay = haystack;
                std::string lower_needle = search_text_;
                std::transform(lower_hay.begin(), lower_hay.end(), lower_hay.begin(), ::tolower);
                std::transform(lower_needle.begin(), lower_needle.end(), lower_needle.begin(), ::tolower);
                return lower_hay.find(lower_needle) != std::string::npos;
            };

            if (matches(s.session_id) ||
                matches(s.project_path) ||
                matches(s.first_prompt) ||
                matches(s.summary.value_or("")) ||
                matches(s.git_branch.value_or("")) ||
                matches(s.note.value_or(""))) {
                filtered.push_back(s);
            } else {
                // Search tags
                for (const auto& tag : s.tags) {
                    if (matches(tag)) {
                        filtered.push_back(s);
                        break;
                    }
                }
            }
        }
        sessions = std::move(filtered);
    }

    // Sort by date (most recent first)
    sessions = SessionStore::sort_by_date(std::move(sessions));

    visible_sessions_ = std::move(sessions);
}

Element TUIApp::render_session(const Session& s, bool selected) {
    std::string star = s.is_favorite ? "* " : "  ";
    std::string sid = s.session_id.substr(0, 8);
    std::string date = format_time(s.modified_at);
    std::string project = project_name(s.project_path);
    std::string branch = s.git_branch.value_or("-");
    std::string status_str = format_status(s.status);
    std::string msgs = std::to_string(s.message_count.value_or(0)) + "msg";
    std::string prompt = truncate(s.display_summary(), 40);

    // Tags line
    std::string tags_str;
    for (std::size_t i = 0; i < s.tags.size(); ++i) {
        if (i > 0) tags_str += " ";
        tags_str += "#" + s.tags[i];
    }

    // First line: star, ID, date, project, branch
    auto line1 = hbox({
        text(star),
        text(sid) | bold,
        text("  "),
        text(date) | dim,
        text("  "),
        text(truncate(project, 16)),
        text("  "),
        text(truncate(branch, 12)) | dim,
        text("  "),
        text(tags_str) | color(Color::Cyan),
    });

    // Second line: prompt, message count, status
    auto line2 = hbox({
        text("    \"" + prompt + "\""),
        text("  "),
        text(msgs) | dim,
        text("  "),
        text(status_str) | (s.status == SessionStatus::active ? color(Color::Green) : color(Color::Red)),
    });

    auto entry = vbox({line1, line2, text("")});

    if (selected) {
        entry = entry | inverted | focus;
    }

    return entry;
}

Element TUIApp::render_info_overlay() {
    if (visible_sessions_.empty()) {
        return text("  세션이 없습니다.") | dim;
    }

    const auto& s = visible_sessions_[static_cast<std::size_t>(selected_index_)];

    Elements info_lines;
    info_lines.push_back(text(" === 세션 정보 === ") | bold);
    info_lines.push_back(hbox({text(" ID:       ") | bold, text(s.session_id)}));
    info_lines.push_back(hbox({text(" 프로젝트: ") | bold, text(s.project_path)}));
    info_lines.push_back(hbox({text(" 브랜치:   ") | bold, text(s.git_branch.value_or("-"))}));
    info_lines.push_back(hbox({text(" 상태:     ") | bold, text(format_status(s.status))}));
    info_lines.push_back(hbox({text(" 메시지:   ") | bold, text(std::to_string(s.message_count.value_or(0)) + "개")}));
    info_lines.push_back(hbox({text(" 생성:     ") | bold, text(format_time(s.created_at))}));
    info_lines.push_back(hbox({text(" 수정:     ") | bold, text(format_time(s.modified_at))}));
    info_lines.push_back(hbox({text(" 요약:     ") | bold, text(s.display_summary())}));

    if (!s.tags.empty()) {
        std::string tags_str;
        for (std::size_t i = 0; i < s.tags.size(); ++i) {
            if (i > 0) tags_str += ", ";
            tags_str += s.tags[i];
        }
        info_lines.push_back(hbox({text(" 태그:     ") | bold, text(tags_str)}));
    }
    if (s.note.has_value() && !s.note->empty()) {
        info_lines.push_back(hbox({text(" 노트:     ") | bold, text(*s.note)}));
    }
    if (s.is_favorite) {
        info_lines.push_back(hbox({text(" 즐겨찾기: ") | bold, text("*")}));
    }

    // Tool usage from JSONL
    if (s.jsonl_path.has_value() && fs::exists(*s.jsonl_path)) {
        auto usage = extract_tool_usage(*s.jsonl_path);
        if (!usage.tool_counts.empty()) {
            info_lines.push_back(text(""));
            info_lines.push_back(text(" --- 도구 사용 ---") | bold);
            for (const auto& [tool, count] : usage.tool_counts) {
                info_lines.push_back(text("   " + tool + ": " + std::to_string(count) + "회"));
            }
        }
        if (!usage.edited_files.empty()) {
            info_lines.push_back(text(""));
            info_lines.push_back(text(" --- 편집 파일 ---") | bold);
            for (const auto& f : usage.edited_files) {
                info_lines.push_back(text("   " + f));
            }
        }
    }

    // Subagent info
    if (s.subagent_count > 0) {
        info_lines.push_back(text(""));
        info_lines.push_back(text(" --- 서브에이전트 ---") | bold);
        info_lines.push_back(text("   개수: " + std::to_string(s.subagent_count)));
        if (!s.subagent_types.empty()) {
            std::string types_str;
            for (std::size_t i = 0; i < s.subagent_types.size(); ++i) {
                if (i > 0) types_str += ", ";
                types_str += s.subagent_types[i];
            }
            info_lines.push_back(text("   유형: " + types_str));
        }
    }

    info_lines.push_back(text(""));
    info_lines.push_back(text(" [i] 또는 [Esc] 닫기") | dim);

    return vbox(std::move(info_lines)) | border | vscroll_indicator | yframe;
}

void TUIApp::handle_resume() {
    if (visible_sessions_.empty()) return;

    const auto& s = visible_sessions_[static_cast<std::size_t>(selected_index_)];
    if (s.status == SessionStatus::expired) return;
    if (!fs::exists(s.project_path)) return;

    std::string cmd = "cd " + s.project_path + " && claude --resume " + s.session_id;
    std::system(cmd.c_str());
}

void TUIApp::handle_tag() {
    if (visible_sessions_.empty()) return;
    input_mode_ = InputMode::tag;
    input_text_.clear();
}

void TUIApp::handle_note() {
    if (visible_sessions_.empty()) return;
    input_mode_ = InputMode::note;
    input_text_.clear();
}

void TUIApp::handle_favorite() {
    if (visible_sessions_.empty()) return;

    auto& s = visible_sessions_[static_cast<std::size_t>(selected_index_)];
    tags_.toggle_favorite(s.session_id);
    tags_.save();

    auto td = tags_.get(s.session_id);
    s.is_favorite = td.has_value() && td->favorite;
}

// --- Helpers ---

std::string TUIApp::format_time(const std::chrono::system_clock::time_point& tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%m-%d %H:%M", std::localtime(&t));
    return buf;
}

std::string TUIApp::format_status(SessionStatus status) {
    switch (status) {
        case SessionStatus::active: return "활성";
        case SessionStatus::expired: return "만료";
    }
    return "unknown";
}

std::string TUIApp::truncate(const std::string& s, std::size_t max_len) {
    if (s.size() <= max_len) return s;
    if (max_len <= 3) return s.substr(0, max_len);
    return s.substr(0, max_len - 3) + "...";
}

std::string TUIApp::project_name(const std::string& project_path) {
    return fs::path(project_path).filename().string();
}

} // namespace ccsm
