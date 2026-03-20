#include <catch2/catch_test_macros.hpp>
#include "core/session_store.hpp"
#include "core/tag_manager.hpp"
#include "util/duration_parser.hpp"
#include <algorithm>

using namespace ccsm;

static std::vector<Session> make_test_sessions() {
    std::vector<Session> sessions;

    Session s1;
    s1.session_id = "aaaa1111-0000-0000-0000-000000000001";
    s1.project_path = "/home/user/projectA";
    s1.first_prompt = "build pipeline";
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
    s2.git_branch = "dev";
    s2.message_count = 10;
    s2.created_at = std::chrono::system_clock::now() - std::chrono::hours(72);
    s2.modified_at = std::chrono::system_clock::now() - std::chrono::hours(48);
    sessions.push_back(s2);

    Session s3;
    s3.session_id = "aaaa3333-0000-0000-0000-000000000003";
    s3.project_path = "/home/user/projectA";
    s3.first_prompt = "add tests";
    s3.status = SessionStatus::active;
    s3.created_at = std::chrono::system_clock::now() - std::chrono::hours(1);
    s3.modified_at = std::chrono::system_clock::now();
    sessions.push_back(s3);

    return sessions;
}

TEST_CASE("SessionStore prefix matching", "[store]") {
    SessionStore store(make_test_sessions());

    SECTION("unique prefix") {
        auto matches = store.find_by_prefix("bbb");
        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].session_id.substr(0, 8) == "bbbb2222");
    }

    SECTION("ambiguous prefix") {
        auto matches = store.find_by_prefix("aaa");
        REQUIRE(matches.size() == 2);
    }

    SECTION("full UUID") {
        auto matches = store.find_by_prefix("aaaa1111-0000-0000-0000-000000000001");
        REQUIRE(matches.size() == 1);
    }

    SECTION("no match") {
        auto matches = store.find_by_prefix("zzzz");
        REQUIRE(matches.empty());
    }
}

TEST_CASE("SessionStore filter_active", "[store]") {
    SessionStore store(make_test_sessions());
    auto active = store.filter_active();
    REQUIRE(active.size() == 2);
    for (const auto& s : active) {
        REQUIRE(s.status == SessionStatus::active);
    }
}

TEST_CASE("SessionStore filter_project", "[store]") {
    SessionStore store(make_test_sessions());
    auto result = store.filter_project("projectA");
    REQUIRE(result.size() == 2);
}

TEST_CASE("SessionStore filter_branch", "[store]") {
    SessionStore store(make_test_sessions());
    auto result = store.filter_branch("dev");
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].session_id.substr(0, 8) == "bbbb2222");
}

TEST_CASE("SessionStore filter_since", "[store]") {
    SessionStore store(make_test_sessions());
    // 36 hours: s1 (modified 24h ago) and s3 (modified now), not s2 (modified 48h ago)
    auto recent = store.filter_since(std::chrono::seconds(36 * 3600));
    REQUIRE(recent.size() == 2);
}

TEST_CASE("SessionStore sort_by_date uses modified_at", "[store]") {
    auto sessions = make_test_sessions();
    auto sorted = SessionStore::sort_by_date(sessions);
    REQUIRE(sorted[0].session_id.substr(0, 8) == "aaaa3333"); // most recent modified_at
    REQUIRE(sorted[2].session_id.substr(0, 8) == "bbbb2222"); // oldest modified_at
}

TEST_CASE("SessionStore sort_by_messages", "[store]") {
    auto sessions = make_test_sessions();
    auto sorted = SessionStore::sort_by_messages(sessions);
    REQUIRE(sorted[0].message_count.value_or(0) == 42);
}

TEST_CASE("SessionStore search basic", "[store]") {
    SessionStore store(make_test_sessions());
    auto results = store.search("pipeline");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].first_prompt == "build pipeline");
}

TEST_CASE("SessionStore search is case-insensitive", "[store]") {
    SessionStore store(make_test_sessions());
    auto results = store.search("PIPELINE");
    REQUIRE(results.size() == 1);
}

TEST_CASE("SessionStore search includes tags", "[store]") {
    auto sessions = make_test_sessions();
    sessions[0].tags = {"training", "idm"};
    SessionStore store(sessions);
    auto results = store.search("training");
    REQUIRE(results.size() == 1);
}

TEST_CASE("SessionStore search includes notes", "[store]") {
    auto sessions = make_test_sessions();
    sessions[0].note = "important pipeline work";
    SessionStore store(sessions);
    auto results = store.search("important");
    REQUIRE(results.size() == 1);
}
