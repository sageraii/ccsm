#include <catch2/catch_test_macros.hpp>
#include "core/session.hpp"

TEST_CASE("Session default construction", "[session]") {
    ccsm::Session s;
    REQUIRE(s.session_id.empty());
    REQUIRE(s.status == ccsm::SessionStatus::expired);
    REQUIRE_FALSE(s.summary.has_value());
    REQUIRE_FALSE(s.message_count.has_value());
    REQUIRE_FALSE(s.git_branch.has_value());
    REQUIRE_FALSE(s.is_favorite);
    REQUIRE(s.subagent_count == 0);
}

TEST_CASE("Session display_summary fallback chain", "[session]") {
    ccsm::Session s;

    SECTION("Level 1: returns summary if present") {
        s.summary = "Built IDM pipeline";
        s.first_prompt = "build idm";
        REQUIRE(s.display_summary() == "Built IDM pipeline");
    }

    SECTION("Level 2: returns first_prompt if no summary") {
        s.first_prompt = "IDM 학습 파이프라인 구축해줘";
        REQUIRE(s.display_summary() == "IDM 학습 파이프라인 구축해줘");
    }

    SECTION("Level 2: skips empty first_prompt") {
        s.first_prompt = "";
        REQUIRE(s.display_summary() == "(내용 없음)");
    }

    SECTION("Level 4: returns fallback if nothing") {
        REQUIRE(s.display_summary() == "(내용 없음)");
    }
}

TEST_CASE("SessionStatus enum", "[session]") {
    REQUIRE(ccsm::SessionStatus::active != ccsm::SessionStatus::expired);
}
