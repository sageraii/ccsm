#include <catch2/catch_test_macros.hpp>
#include "core/jsonl_parser.hpp"
#include <filesystem>

using namespace ccsm;
namespace fs = std::filesystem;

TEST_CASE("extract_session_meta from progress-first", "[jsonl]") {
    auto meta = extract_session_meta(fs::path("fixtures/session-progress-first.jsonl"));
    REQUIRE(meta.has_value());
    REQUIRE(meta->session_id == "aaaa1111-0000-0000-0000-000000000001");
    REQUIRE(meta->cwd == "/home/user/project");
    REQUIRE(meta->git_branch == "main");
}

TEST_CASE("extract_session_meta scan-forward snapshot-first", "[jsonl]") {
    auto meta = extract_session_meta(fs::path("fixtures/session-snapshot-first.jsonl"));
    REQUIRE(meta.has_value());
    REQUIRE(meta->session_id == "bbbb2222-0000-0000-0000-000000000002");
    REQUIRE(meta->cwd == "/home/user/ucp");
}

TEST_CASE("extract_session_meta scan-forward multi-snapshot", "[jsonl]") {
    auto meta = extract_session_meta(fs::path("fixtures/session-multi-snapshot.jsonl"));
    REQUIRE(meta.has_value());
    REQUIRE(meta->session_id == "cccc3333-0000-0000-0000-000000000003");
}

TEST_CASE("extract_session_meta returns nullopt for snapshot-only", "[jsonl]") {
    auto meta = extract_session_meta(fs::path("fixtures/session-snapshot-only.jsonl"));
    REQUIRE_FALSE(meta.has_value());
}

TEST_CASE("extract_tool_usage from JSONL with tools", "[jsonl]") {
    auto usage = extract_tool_usage(fs::path("fixtures/session-with-tools.jsonl"));
    REQUIRE(usage.edited_files.size() == 1);
    REQUIRE(usage.edited_files[0] == "/src/main.cpp");
    REQUIRE(usage.created_files.size() == 1);
    REQUIRE(usage.created_files[0] == "/src/new.cpp");
    REQUIRE(usage.tool_counts.at("Edit") == 1);
    REQUIRE(usage.tool_counts.at("Write") == 1);
    REQUIRE(usage.tool_counts.at("Bash") == 1);
}

TEST_CASE("extract_first_user_prompt finds string content", "[jsonl]") {
    auto prompt = extract_first_user_prompt(fs::path("fixtures/session-with-tools.jsonl"));
    REQUIRE(prompt.has_value());
    REQUIRE(prompt.value() == "fix the bug");
}

TEST_CASE("extract_first_user_prompt skips tool_result arrays", "[jsonl]") {
    auto prompt = extract_first_user_prompt(fs::path("fixtures/session-with-tools.jsonl"));
    REQUIRE(prompt.has_value());
    REQUIRE(prompt.value() == "fix the bug"); // Not the tool_result
}

TEST_CASE("extract_session_meta handles missing file", "[jsonl]") {
    auto meta = extract_session_meta(fs::path("nonexistent.jsonl"));
    REQUIRE_FALSE(meta.has_value());
}
