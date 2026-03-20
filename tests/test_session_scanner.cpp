#include <catch2/catch_test_macros.hpp>
#include "core/session_scanner.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

using namespace ccsm;
namespace fs = std::filesystem;

TEST_CASE("SessionScanner merges history sessions", "[scanner]") {
    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");
    config.project_dirs = {};

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    REQUIRE(sessions.size() == 4); // 4 unique sessions in fixture
}

TEST_CASE("SessionScanner deduplicates by sessionId", "[scanner]") {
    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    std::set<std::string> ids;
    for (const auto& s : sessions) ids.insert(s.session_id);
    REQUIRE(ids.size() == sessions.size());
}

TEST_CASE("SessionScanner marks JSONL sessions as active", "[scanner]") {
    // Create temp project dir with a JSONL file matching a history session
    auto tmp = fs::temp_directory_path() / "ccsm_scanner_test";
    fs::create_directories(tmp);

    auto jsonl = tmp / "aaaa1111-0000-0000-0000-000000000001.jsonl";
    {
        std::ofstream f(jsonl);
        f << R"({"type":"progress","sessionId":"aaaa1111-0000-0000-0000-000000000001","cwd":"/tmp","gitBranch":"","timestamp":"2026-01-24T00:00:00Z","uuid":"u","parentUuid":null,"isSidechain":false,"data":{},"toolUseID":"t","parentToolUseID":"t","userType":"external","slug":"s","version":"2.1.79"})" << "\n";
    }

    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");
    config.project_dirs = {tmp};

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "aaaa1111"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->status == SessionStatus::active);
    REQUIRE(it->jsonl_path.has_value());

    fs::remove_all(tmp);
}

TEST_CASE("SessionScanner discovers orphan session directories", "[scanner]") {
    auto tmp = fs::temp_directory_path() / "ccsm_scanner_orphan";
    auto orphan_dir = tmp / "ffff6666-0000-0000-0000-000000000006" / "subagents";
    fs::create_directories(orphan_dir);

    // Create a subagent file
    std::ofstream(orphan_dir / "agent-a123.jsonl") << "{}\n";

    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");
    config.project_dirs = {tmp};

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "ffff6666"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->status == SessionStatus::active);
    REQUIRE(it->subagent_count == 1);

    fs::remove_all(tmp);
}

TEST_CASE("SessionScanner merges index data into history sessions", "[scanner]") {
    auto tmp = fs::temp_directory_path() / "ccsm_scanner_index";
    fs::create_directories(tmp);

    // Copy index fixture
    fs::copy_file(fs::path("fixtures/sessions-index-normal.json"),
                  tmp / "sessions-index.json",
                  fs::copy_options::overwrite_existing);

    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");
    config.project_dirs = {tmp};

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    // aaaa1111 is in both history and index — should get summary from index
    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "aaaa1111"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->summary.has_value());
    REQUIRE(it->summary.value() == "Built IDM pipeline");
    REQUIRE(it->message_count.has_value());
    REQUIRE(it->message_count.value() == 42);

    fs::remove_all(tmp);
}

TEST_CASE("SessionScanner history-only sessions are expired", "[scanner]") {
    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");
    config.project_dirs = {}; // no project dirs

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    for (const auto& s : sessions) {
        REQUIRE(s.status == SessionStatus::expired);
    }
}

TEST_CASE("SessionScanner detects subagent meta.json", "[scanner]") {
    auto tmp = fs::temp_directory_path() / "ccsm_scanner_meta";
    auto sub_dir = tmp / "aaaa1111-0000-0000-0000-000000000001" / "subagents";
    fs::create_directories(sub_dir);

    std::ofstream(sub_dir / "agent-a100.jsonl") << "{}\n";
    std::ofstream(sub_dir / "agent-a200.jsonl") << "{}\n";
    std::ofstream(sub_dir / "agent-acompact-300.jsonl") << "{}\n"; // compact, should not count
    std::ofstream(sub_dir / "agent-a100.meta.json") << R"({"agentType":"Explore","description":"test"})";

    ScanConfig config;
    config.history_path = fs::path("fixtures/history_sample.jsonl");
    config.project_dirs = {tmp};

    SessionScanner scanner(config);
    auto sessions = scanner.scan();

    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "aaaa1111"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->status == SessionStatus::active);
    REQUIRE(it->subagent_count == 2); // 2 regular, not compact
    REQUIRE(it->subagent_types.size() == 1);
    REQUIRE(it->subagent_types[0] == "Explore");

    fs::remove_all(tmp);
}
