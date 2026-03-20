#include <catch2/catch_test_macros.hpp>
#include "util/text_sanitizer.hpp"

using namespace ccsm;

TEST_CASE("sanitize_prompt strips XML command tags", "[sanitizer]") {
    SECTION("extracts command-args content") {
        auto input = R"(<command-message>superpowers:brainstorm</command-message>)"
                     R"(<command-name>/superpowers:brainstorm</command-name>)"
                     R"(<command-args>in korea</command-args>)";
        REQUIRE(sanitize_prompt(input) == "in korea");
    }

    SECTION("returns plain text unchanged") {
        REQUIRE(sanitize_prompt("IDM 학습 파이프라인 구축해줘") == "IDM 학습 파이프라인 구축해줘");
    }

    SECTION("truncates at 200 chars") {
        std::string long_text(250, 'A');
        auto result = sanitize_prompt(long_text);
        REQUIRE(result.size() == 203); // 200 + "..."
        REQUIRE(result.substr(200) == "...");
    }
}

TEST_CASE("is_sentinel returns true for sentinel values", "[sanitizer]") {
    REQUIRE(is_sentinel("No prompt"));
    REQUIRE(is_sentinel(""));
    REQUIRE_FALSE(is_sentinel("actual prompt"));
}

TEST_CASE("strip_xml_tags removes all XML tags", "[sanitizer]") {
    REQUIRE(strip_xml_tags("<b>bold</b>") == "bold");
    REQUIRE(strip_xml_tags("no tags") == "no tags");
    REQUIRE(strip_xml_tags("<a><b>nested</b></a>") == "nested");
}

TEST_CASE("sanitize_prompt returns empty for sentinels", "[sanitizer]") {
    // Critical: sentinel should return empty so display_summary() falls through
    REQUIRE(sanitize_prompt("No prompt").empty());
    REQUIRE(sanitize_prompt("").empty());
}
