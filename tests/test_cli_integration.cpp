#include <catch2/catch_test_macros.hpp>
#include "core/session.hpp"
#include "core/session_store.hpp"
#include "core/tag_manager.hpp"
#include "cli/cli_app.hpp"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <iostream>

using namespace ccsm;
namespace fs = std::filesystem;

static std::vector<Session> make_cli_sessions() {
    std::vector<Session> sessions;

    Session s1;
    s1.session_id = "aaaa1111-0000-0000-0000-000000000001";
    s1.project_path = "/home/user/projectA";
    s1.first_prompt = "build pipeline";
    s1.summary = "Built IDM pipeline";
    s1.status = SessionStatus::active;
    s1.git_branch = "main";
    s1.message_count = 42;
    s1.created_at = std::chrono::system_clock::now() - std::chrono::hours(48);
    s1.modified_at = std::chrono::system_clock::now() - std::chrono::hours(24);
    sessions.push_back(s1);

    Session s2;
    s2.session_id = "bbbb2222-0000-0000-0000-000000000002";
    s2.project_path = "/home/user/projectB";
    s2.first_prompt = "fix bug";
    s2.status = SessionStatus::expired;
    s2.created_at = std::chrono::system_clock::now() - std::chrono::hours(72);
    s2.modified_at = std::chrono::system_clock::now() - std::chrono::hours(48);
    sessions.push_back(s2);

    return sessions;
}

// Helper: capture stdout from CLI run
static std::pair<int, std::string> run_cli(SessionStore& store, TagManager& tags,
                                            std::vector<std::string> args) {
    std::vector<char*> argv;
    std::string prog = "ccsm";
    argv.push_back(prog.data());
    for (auto& a : args) argv.push_back(a.data());

    std::ostringstream capture;
    auto* old_buf = std::cout.rdbuf(capture.rdbuf());

    CLIApp cli(store, tags);
    int rc = cli.run(static_cast<int>(argv.size()), argv.data());

    std::cout.rdbuf(old_buf);
    return {rc, capture.str()};
}

TEST_CASE("CLI list shows active sessions by default", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags1.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"list"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("aaaa1111") != std::string::npos);
    REQUIRE(out.find("bbbb2222") == std::string::npos); // expired, hidden
    fs::remove(tmp);
}

TEST_CASE("CLI list --expired shows all", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags2.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"list", "--expired"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("aaaa1111") != std::string::npos);
    REQUIRE(out.find("bbbb2222") != std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("CLI list --format json", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags3.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"list", "--expired", "--format", "json"});
    REQUIRE(rc == 0);
    REQUIRE(out.front() == '[');
    REQUIRE(out.find("session_id") != std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("CLI list --project filters", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags4.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"list", "--expired", "--project", "projectA"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("aaaa1111") != std::string::npos);
    REQUIRE(out.find("bbbb2222") == std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("CLI resume rejects expired session", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags5.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"resume", "bbbb"});
    REQUIRE(rc != 0);
    REQUIRE(out.find("만료") != std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("CLI resume rejects unknown session", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags6.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"resume", "zzzz"});
    REQUIRE(rc != 0);
    fs::remove(tmp);
}

TEST_CASE("CLI tag adds tag", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags7.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"tag", "aaaa", "myTag"});
    REQUIRE(rc == 0);
    auto td = tags.get("aaaa1111-0000-0000-0000-000000000001");
    REQUIRE(td.has_value());
    REQUIRE(std::find(td->tags.begin(), td->tags.end(), "myTag") != td->tags.end());
    fs::remove(tmp);
}

TEST_CASE("CLI search finds by keyword", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags8.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"search", "pipeline"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("aaaa1111") != std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("CLI info shows session detail", "[cli]") {
    SessionStore store(make_cli_sessions());
    auto tmp = fs::temp_directory_path() / "cli_test_tags9.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"info", "aaaa"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("aaaa1111") != std::string::npos);
    REQUIRE(out.find("projectA") != std::string::npos);
    REQUIRE(out.find("Built IDM pipeline") != std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("AC15: no XML markup in display output", "[cli][ac15]") {
    std::vector<Session> sessions;
    Session s;
    s.session_id = "ffff6666-0000-0000-0000-000000000006";
    s.project_path = "/home/user/test";
    s.first_prompt = ""; // sanitized from "No prompt"
    s.status = SessionStatus::active;
    s.modified_at = std::chrono::system_clock::now();
    s.created_at = s.modified_at;
    sessions.push_back(s);

    SessionStore store(sessions);
    auto tmp = fs::temp_directory_path() / "ac15_tags.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"list", "--format", "json"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("<command") == std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("AC16: --version outputs version", "[cli][ac16]") {
    SessionStore store({});
    auto tmp = fs::temp_directory_path() / "ac16_tags.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"--version"});
    REQUIRE(rc == 0);
    REQUIRE(out.find("0.1.0") != std::string::npos);
    fs::remove(tmp);
}

TEST_CASE("AC6: info shows subagent info when present", "[cli][ac6]") {
    std::vector<Session> sessions;
    Session s;
    s.session_id = "aaaa1111-0000-0000-0000-000000000001";
    s.project_path = "/tmp";
    s.first_prompt = "test";
    s.status = SessionStatus::active;
    s.subagent_count = 3;
    s.subagent_types = {"Explore", "general-purpose"};
    s.modified_at = std::chrono::system_clock::now();
    s.created_at = s.modified_at;
    sessions.push_back(s);

    SessionStore store(sessions);
    auto tmp = fs::temp_directory_path() / "ac6_tags.json";
    TagManager tags(tmp);

    auto [rc, out] = run_cli(store, tags, {"info", "aaaa"});
    REQUIRE(rc == 0);
    // Should show subagent info
    REQUIRE(out.find("3") != std::string::npos); // subagent count
    fs::remove(tmp);
}
