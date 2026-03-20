#include "cli/cli_app.hpp"
#include "util/duration_parser.hpp"
#include "core/jsonl_parser.hpp"
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <filesystem>

namespace ccsm {

using json = nlohmann::json;
namespace fs = std::filesystem;

CLIApp::CLIApp(SessionStore& store, TagManager& tags)
    : store_(store), tags_(tags) {}

int CLIApp::run(int argc, char* argv[]) {
    CLI::App app{"ccsm - Claude Code Session Manager"};
    app.set_version_flag("--version", "ccsm v0.1.0");
    app.require_subcommand(0, 1);

    // --- list ---
    bool list_expired = false;
    bool list_all = false;
    std::string list_project, list_branch, list_since, list_tag;
    std::string list_sort = "date";
    std::string list_format = "table";
    int list_limit = 20;

    auto* sub_list = app.add_subcommand("list", "세션 목록 조회");
    sub_list->add_flag("--expired", list_expired, "만료된 세션 포함");
    sub_list->add_flag("--all,-a", list_all, "제한 없이 전체 표시");
    sub_list->add_option("--project,-p", list_project, "프로젝트 필터");
    sub_list->add_option("--branch,-b", list_branch, "브랜치 필터");
    sub_list->add_option("--since,-s", list_since, "기간 필터 (예: 2d, 1w)");
    sub_list->add_option("--tag,-t", list_tag, "태그 필터");
    sub_list->add_option("--sort", list_sort, "정렬 기준 (date|messages|project)");
    sub_list->add_option("--format,-f", list_format, "출력 형식 (table|json|csv)");
    sub_list->add_option("--limit,-l", list_limit, "표시 개수 제한 (기본: 20)");
    sub_list->callback([&]() {
        cmd_list(list_expired, list_all, list_project, list_branch,
                 list_since, list_tag, list_sort, list_format, list_limit);
    });

    // --- info ---
    std::string info_prefix;
    auto* sub_info = app.add_subcommand("info", "세션 상세 정보");
    sub_info->add_option("session-id", info_prefix, "세션 ID (prefix)")->required();
    sub_info->callback([&]() { cmd_info(info_prefix); });

    // --- search ---
    std::string search_keyword;
    bool search_deep = false;
    auto* sub_search = app.add_subcommand("search", "세션 검색");
    sub_search->add_option("keyword", search_keyword, "검색어")->required();
    sub_search->add_flag("--deep", search_deep, "JSONL 내용 검색 (예정)");
    sub_search->callback([&]() { cmd_search(search_keyword, search_deep); });

    // --- tag ---
    std::string tag_prefix, tag_value;
    auto* sub_tag = app.add_subcommand("tag", "태그 추가");
    sub_tag->add_option("session-id", tag_prefix, "세션 ID (prefix)")->required();
    sub_tag->add_option("tag", tag_value, "태그")->required();
    sub_tag->callback([&]() { cmd_tag(tag_prefix, tag_value); });

    // --- untag ---
    std::string untag_prefix, untag_value;
    auto* sub_untag = app.add_subcommand("untag", "태그 제거");
    sub_untag->add_option("session-id", untag_prefix, "세션 ID (prefix)")->required();
    sub_untag->add_option("tag", untag_value, "태그")->required();
    sub_untag->callback([&]() { cmd_untag(untag_prefix, untag_value); });

    // --- note ---
    std::string note_prefix, note_text;
    auto* sub_note = app.add_subcommand("note", "노트 설정");
    sub_note->add_option("session-id", note_prefix, "세션 ID (prefix)")->required();
    sub_note->add_option("text", note_text, "노트 내용")->required();
    sub_note->callback([&]() { cmd_note(note_prefix, note_text); });

    // --- favorite ---
    std::string fav_prefix;
    auto* sub_fav = app.add_subcommand("favorite", "즐겨찾기 토글");
    sub_fav->add_option("session-id", fav_prefix, "세션 ID (prefix)")->required();
    sub_fav->callback([&]() { cmd_favorite(fav_prefix); });

    // --- favorites ---
    auto* sub_favs = app.add_subcommand("favorites", "즐겨찾기 목록");
    sub_favs->callback([&]() { cmd_favorites(); });

    // --- resume ---
    std::string resume_prefix;
    auto* sub_resume = app.add_subcommand("resume", "세션 재개");
    sub_resume->add_option("session-id", resume_prefix, "세션 ID (prefix)")->required();
    sub_resume->callback([&]() { cmd_resume(resume_prefix); });

    // --- cleanup ---
    auto* sub_cleanup = app.add_subcommand("cleanup", "오래된 태그 데이터 정리");
    sub_cleanup->callback([&]() { cmd_cleanup(); });

    try {
        app.parse(argc, argv);
    } catch (const CLIError& e) {
        std::cout << e.what() << "\n";
        return e.exit_code;
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    return 0;
}

// --- Subcommand implementations ---

void CLIApp::cmd_list(bool expired, bool all, const std::string& project,
                      const std::string& branch, const std::string& since,
                      const std::string& tag, const std::string& sort,
                      const std::string& format, int limit) {
    std::vector<Session> sessions;

    if (expired) {
        sessions = store_.all();
    } else {
        sessions = store_.filter_active();
    }

    // Apply filters
    if (!project.empty()) {
        std::vector<Session> filtered;
        for (const auto& s : sessions) {
            if (s.project_path.find(project) != std::string::npos) {
                filtered.push_back(s);
            }
        }
        sessions = std::move(filtered);
    }

    if (!branch.empty()) {
        std::vector<Session> filtered;
        for (const auto& s : sessions) {
            if (s.git_branch.has_value() && *s.git_branch == branch) {
                filtered.push_back(s);
            }
        }
        sessions = std::move(filtered);
    }

    if (!since.empty()) {
        auto dur = parse_duration(since);
        if (!dur.has_value()) {
            throw CLIError("잘못된 기간 형식: " + since + " (" + supported_formats_hint() + ")");
        }
        auto cutoff = std::chrono::system_clock::now() - *dur;
        std::vector<Session> filtered;
        for (const auto& s : sessions) {
            if (s.modified_at >= cutoff) {
                filtered.push_back(s);
            }
        }
        sessions = std::move(filtered);
    }

    if (!tag.empty()) {
        auto tagged_ids = tags_.find_by_tag(tag);
        std::vector<Session> filtered;
        for (const auto& s : sessions) {
            if (std::find(tagged_ids.begin(), tagged_ids.end(), s.session_id) != tagged_ids.end()) {
                filtered.push_back(s);
            }
        }
        sessions = std::move(filtered);
    }

    // Sort
    if (sort == "date") {
        sessions = SessionStore::sort_by_date(std::move(sessions));
    } else if (sort == "messages") {
        sessions = SessionStore::sort_by_messages(std::move(sessions));
    } else if (sort == "project") {
        sessions = SessionStore::sort_by_project(std::move(sessions));
    }

    // Limit (unless --all)
    if (!all && static_cast<int>(sessions.size()) > limit) {
        sessions.resize(static_cast<std::size_t>(limit));
    }

    // Output
    if (format == "json") {
        print_json(sessions);
    } else if (format == "csv") {
        print_csv(sessions);
    } else {
        print_table(sessions);
    }
}

void CLIApp::cmd_info(const std::string& prefix) {
    auto session = resolve_session(prefix);

    std::cout << "=== 세션 정보 ===\n";
    std::cout << "ID:        " << session.session_id << "\n";
    std::cout << "프로젝트:  " << session.project_path << "\n";
    std::cout << "브랜치:    " << session.git_branch.value_or("-") << "\n";
    std::cout << "상태:      " << format_status(session.status) << "\n";
    std::cout << "메시지:    " << session.message_count.value_or(0) << "개\n";
    std::cout << "생성:      " << format_time(session.created_at) << "\n";
    std::cout << "수정:      " << format_time(session.modified_at) << "\n";
    std::cout << "요약:      " << session.display_summary() << "\n";

    if (!session.tags.empty()) {
        std::cout << "태그:      ";
        for (std::size_t i = 0; i < session.tags.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << session.tags[i];
        }
        std::cout << "\n";
    }
    if (session.note.has_value() && !session.note->empty()) {
        std::cout << "노트:      " << *session.note << "\n";
    }
    if (session.is_favorite) {
        std::cout << "즐겨찾기:  ★\n";
    }

    // Tool usage from JSONL
    if (session.jsonl_path.has_value() && fs::exists(*session.jsonl_path)) {
        auto usage = extract_tool_usage(*session.jsonl_path);
        if (!usage.tool_counts.empty()) {
            std::cout << "\n--- 도구 사용 ---\n";
            for (const auto& [tool, count] : usage.tool_counts) {
                std::cout << "  " << tool << ": " << count << "회\n";
            }
        }
        if (!usage.edited_files.empty()) {
            std::cout << "\n--- 편집 파일 ---\n";
            for (const auto& f : usage.edited_files) {
                std::cout << "  " << f << "\n";
            }
        }
    }

    // Subagent info
    if (session.subagent_count > 0) {
        std::cout << "\n--- 서브에이전트 ---\n";
        std::cout << "  개수: " << session.subagent_count << "\n";
        if (!session.subagent_types.empty()) {
            std::cout << "  유형: ";
            for (std::size_t i = 0; i < session.subagent_types.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << session.subagent_types[i];
            }
            std::cout << "\n";
        }
    }
}

void CLIApp::cmd_search(const std::string& keyword, bool deep) {
    if (deep) {
        std::cout << "v0.2.0에서 지원 예정\n";
        return;
    }

    auto results = store_.search(keyword);
    if (results.empty()) {
        std::cout << "검색 결과 없음: " << keyword << "\n";
        return;
    }
    print_table(results);
}

void CLIApp::cmd_tag(const std::string& prefix, const std::string& tag) {
    auto session = resolve_session(prefix);
    tags_.add_tag(session.session_id, tag);
    tags_.save();
    std::cout << "태그 추가: " << session.session_id.substr(0, 8) << " <- " << tag << "\n";
}

void CLIApp::cmd_untag(const std::string& prefix, const std::string& tag) {
    auto session = resolve_session(prefix);
    tags_.remove_tag(session.session_id, tag);
    tags_.save();
    std::cout << "태그 제거: " << session.session_id.substr(0, 8) << " <- " << tag << "\n";
}

void CLIApp::cmd_note(const std::string& prefix, const std::string& text) {
    auto session = resolve_session(prefix);
    tags_.set_note(session.session_id, text);
    tags_.save();
    std::cout << "노트 설정: " << session.session_id.substr(0, 8) << "\n";
}

void CLIApp::cmd_favorite(const std::string& prefix) {
    auto session = resolve_session(prefix);
    tags_.toggle_favorite(session.session_id);
    tags_.save();

    auto td = tags_.get(session.session_id);
    if (td.has_value() && td->favorite) {
        std::cout << "★ 즐겨찾기 추가: " << session.session_id.substr(0, 8) << "\n";
    } else {
        std::cout << "☆ 즐겨찾기 해제: " << session.session_id.substr(0, 8) << "\n";
    }
}

void CLIApp::cmd_favorites() {
    auto fav_ids = tags_.get_favorites();
    if (fav_ids.empty()) {
        std::cout << "즐겨찾기가 없습니다.\n";
        return;
    }

    std::vector<Session> fav_sessions;
    for (const auto& fid : fav_ids) {
        auto matches = store_.find_by_prefix(fid);
        for (auto& s : matches) {
            fav_sessions.push_back(std::move(s));
        }
    }

    if (fav_sessions.empty()) {
        std::cout << "즐겨찾기 세션을 찾을 수 없습니다.\n";
        return;
    }

    fav_sessions = SessionStore::sort_by_date(std::move(fav_sessions));
    print_table(fav_sessions);
}

void CLIApp::cmd_resume(const std::string& prefix) {
    auto session = resolve_session(prefix);

    if (session.status == SessionStatus::expired) {
        throw CLIError("만료된 세션은 재개할 수 없습니다: " + session.session_id.substr(0, 8));
    }

    if (!fs::exists(session.project_path)) {
        throw CLIError("프로젝트 디렉토리가 존재하지 않습니다: " + session.project_path);
    }

    std::string cmd = "cd " + session.project_path + " && claude --resume " + session.session_id;
    std::cout << "세션 재개: " << session.session_id.substr(0, 8) << "\n";
    std::cout << "$ " << cmd << "\n";
    std::system(cmd.c_str());
}

void CLIApp::cmd_cleanup() {
    auto active = store_.filter_active();
    std::vector<std::string> active_ids;
    active_ids.reserve(active.size());
    for (const auto& s : active) {
        active_ids.push_back(s.session_id);
    }

    tags_.cleanup(active_ids);
    tags_.save();
    std::cout << "정리 완료\n";
}

// --- resolve_session ---

Session CLIApp::resolve_session(const std::string& prefix) {
    if (prefix.size() < 3) {
        throw CLIError("세션 ID prefix는 최소 3자 이상 필요합니다.");
    }

    auto matches = store_.find_by_prefix(prefix);

    if (matches.empty()) {
        throw CLIError("세션을 찾을 수 없습니다: " + prefix);
    }

    if (matches.size() > 1) {
        std::string msg = "여러 세션이 일치합니다:\n";
        for (const auto& s : matches) {
            msg += "  " + s.session_id.substr(0, 8) + " - " + s.display_summary() + "\n";
        }
        throw CLIError(msg);
    }

    // Apply tag data to the matched session
    auto& session = matches[0];
    auto td = tags_.get(session.session_id);
    if (td.has_value()) {
        session.tags = td->tags;
        session.note = td->note.empty() ? std::nullopt : std::optional<std::string>(td->note);
        session.is_favorite = td->favorite;
    }

    return session;
}

// --- Output formatters ---

void CLIApp::print_table(const std::vector<Session>& sessions) {
    if (sessions.empty()) {
        std::cout << "표시할 세션이 없습니다.\n";
        return;
    }

    // Header
    std::cout << std::left
              << "  "
              << std::setw(10) << "ID"
              << std::setw(12) << "날짜"
              << std::setw(16) << "프로젝트"
              << std::setw(10) << "브랜치"
              << std::setw(8)  << "상태"
              << std::setw(5)  << "Msg"
              << std::setw(12) << "태그"
              << "내용"
              << "\n";

    std::cout << std::string(100, '-') << "\n";

    for (const auto& s : sessions) {
        std::string star = s.is_favorite ? "★ " : "  ";
        std::string sid = s.session_id.substr(0, 8);
        std::string date = format_time(s.modified_at);
        std::string project = truncate(fs::path(s.project_path).filename().string(), 14);
        std::string branch = truncate(s.git_branch.value_or("-"), 8);
        std::string status = format_status(s.status);
        std::string msgs = std::to_string(s.message_count.value_or(0));
        std::string tags_str;
        for (std::size_t i = 0; i < s.tags.size(); ++i) {
            if (i > 0) tags_str += ",";
            tags_str += s.tags[i];
        }
        tags_str = truncate(tags_str, 10);
        std::string prompt = truncate(s.display_summary(), 30);

        std::cout << star
                  << std::setw(10) << sid
                  << std::setw(12) << date
                  << std::setw(16) << project
                  << std::setw(10) << branch
                  << std::setw(8)  << status
                  << std::setw(5)  << msgs
                  << std::setw(12) << tags_str
                  << prompt
                  << "\n";
    }
}

void CLIApp::print_json(const std::vector<Session>& sessions) {
    json arr = json::array();

    for (const auto& s : sessions) {
        json obj;
        obj["session_id"] = s.session_id;
        obj["project_path"] = s.project_path;
        obj["first_prompt"] = s.first_prompt;
        obj["summary"] = s.summary.value_or("");
        obj["status"] = format_status(s.status);
        obj["git_branch"] = s.git_branch.value_or("");
        obj["message_count"] = s.message_count.value_or(0);
        obj["is_favorite"] = s.is_favorite;
        obj["tags"] = s.tags;
        obj["note"] = s.note.value_or("");

        auto to_iso = [](const std::chrono::system_clock::time_point& tp) {
            auto t = std::chrono::system_clock::to_time_t(tp);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%FT%TZ", std::gmtime(&t));
            return std::string(buf);
        };

        obj["created_at"] = to_iso(s.created_at);
        obj["modified_at"] = to_iso(s.modified_at);
        arr.push_back(obj);
    }

    std::cout << arr.dump(2) << "\n";
}

void CLIApp::print_csv(const std::vector<Session>& sessions) {
    std::cout << "session_id,project_path,branch,status,message_count,prompt\n";

    for (const auto& s : sessions) {
        // Escape fields with commas/quotes
        auto escape = [](const std::string& field) -> std::string {
            if (field.find(',') != std::string::npos ||
                field.find('"') != std::string::npos) {
                std::string escaped = field;
                // Double up any existing quotes
                std::size_t pos = 0;
                while ((pos = escaped.find('"', pos)) != std::string::npos) {
                    escaped.insert(pos, "\"");
                    pos += 2;
                }
                return "\"" + escaped + "\"";
            }
            return field;
        };

        std::cout << s.session_id << ","
                  << escape(s.project_path) << ","
                  << escape(s.git_branch.value_or("")) << ","
                  << format_status(s.status) << ","
                  << s.message_count.value_or(0) << ","
                  << escape(s.display_summary()) << "\n";
    }
}

// --- Helpers ---

std::string CLIApp::format_time(const std::chrono::system_clock::time_point& tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%m-%d %H:%M", std::localtime(&t));
    return buf;
}

std::string CLIApp::format_status(SessionStatus status) {
    switch (status) {
        case SessionStatus::active: return "active";
        case SessionStatus::expired: return "expired";
    }
    return "unknown";
}

std::string CLIApp::truncate(const std::string& s, std::size_t max_len) {
    if (s.size() <= max_len) return s;
    if (max_len <= 3) return s.substr(0, max_len);
    return s.substr(0, max_len - 3) + "...";
}

} // namespace ccsm
