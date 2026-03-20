#include <catch2/catch_test_macros.hpp>
#include "core/history_parser.hpp"
#include <filesystem>
#include <algorithm>

using namespace ccsm;
namespace fs = std::filesystem;

static const auto FIXTURE = fs::path("fixtures/history_sample.jsonl");

TEST_CASE("parse_history extracts unique sessions", "[history]") {
    auto sessions = parse_history(FIXTURE);
    REQUIRE(sessions.size() == 4); // 5 lines, 4 unique sessionIds
}

TEST_CASE("parse_history populates fields correctly", "[history]") {
    auto sessions = parse_history(FIXTURE);
    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "aaaa1111"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->project_path == "/home/user/GR00T");
    REQUIRE(it->first_prompt == "IDM 학습 파이프라인 구축해줘");
}

TEST_CASE("parse_history uses earliest prompt per session", "[history]") {
    auto sessions = parse_history(FIXTURE);
    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "aaaa1111"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->first_prompt == "IDM 학습 파이프라인 구축해줘"); // Not "두번째 프롬프트"
}

TEST_CASE("parse_history tracks modified_at as latest timestamp", "[history]") {
    auto sessions = parse_history(FIXTURE);
    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "aaaa1111"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->created_at < it->modified_at); // 2 entries with different timestamps
}

TEST_CASE("parse_history sanitizes command markup", "[history]") {
    auto sessions = parse_history(FIXTURE);
    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "cccc3333"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->first_prompt == "test"); // command-args extracted
}

TEST_CASE("parse_history returns empty for sentinel prompts", "[history]") {
    auto sessions = parse_history(FIXTURE);
    auto it = std::find_if(sessions.begin(), sessions.end(),
        [](const auto& s) { return s.session_id.substr(0, 8) == "dddd4444"; });
    REQUIRE(it != sessions.end());
    REQUIRE(it->first_prompt.empty()); // "No prompt" sanitized to empty
}

TEST_CASE("parse_history handles missing file gracefully", "[history]") {
    auto sessions = parse_history(fs::path("nonexistent.jsonl"));
    REQUIRE(sessions.empty());
}
