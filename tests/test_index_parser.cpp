#include <catch2/catch_test_macros.hpp>
#include "core/index_parser.hpp"
#include <filesystem>

using namespace ccsm;
namespace fs = std::filesystem;

TEST_CASE("parse_index extracts entries", "[index]") {
    auto result = parse_index(fs::path("fixtures/sessions-index-normal.json"));
    REQUIRE(result.entries.size() == 2);
    REQUIRE(result.original_path.has_value());
    REQUIRE(result.original_path.value() == "/home/user/project");
}

TEST_CASE("parse_index handles summary as optional", "[index]") {
    auto result = parse_index(fs::path("fixtures/sessions-index-normal.json"));
    REQUIRE(result.entries[0].summary.has_value());
    REQUIRE(result.entries[0].summary.value() == "Built IDM pipeline");
    REQUIRE_FALSE(result.entries[1].summary.has_value());
}

TEST_CASE("parse_index reads all fields", "[index]") {
    auto result = parse_index(fs::path("fixtures/sessions-index-normal.json"));
    auto& e = result.entries[0];
    REQUIRE(e.session_id == "aaaa1111-0000-0000-0000-000000000001");
    REQUIRE(e.message_count == 42);
    REQUIRE(e.git_branch == "main");
    REQUIRE(e.project_path == "/home/user/project");
    REQUIRE_FALSE(e.is_sidechain);
}

TEST_CASE("parse_index handles empty gitBranch", "[index]") {
    auto result = parse_index(fs::path("fixtures/sessions-index-normal.json"));
    REQUIRE(result.entries[1].git_branch.empty());
}

TEST_CASE("parse_index empty entries array", "[index]") {
    auto result = parse_index(fs::path("fixtures/sessions-index-empty.json"));
    REQUIRE(result.entries.empty());
    REQUIRE(result.original_path == "/home/user/empty");
}

TEST_CASE("parse_index missing originalPath", "[index]") {
    auto result = parse_index(fs::path("fixtures/sessions-index-no-original.json"));
    REQUIRE_FALSE(result.original_path.has_value());
    REQUIRE(result.entries.size() == 1);
    REQUIRE(result.entries[0].git_branch == "dev");
}

TEST_CASE("parse_index handles missing file", "[index]") {
    auto result = parse_index(fs::path("nonexistent.json"));
    REQUIRE(result.entries.empty());
    REQUIRE_FALSE(result.original_path.has_value());
}
