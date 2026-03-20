#include <catch2/catch_test_macros.hpp>
#include "util/duration_parser.hpp"

using namespace ccsm;

TEST_CASE("parse_duration handles valid inputs", "[duration]") {
    SECTION("days") {
        auto d = parse_duration("7d");
        REQUIRE(d.has_value());
        REQUIRE(d->count() == 7 * 24 * 3600);
    }
    SECTION("weeks") {
        auto d = parse_duration("2w");
        REQUIRE(d.has_value());
        REQUIRE(d->count() == 14 * 24 * 3600);
    }
    SECTION("months as 30 days") {
        auto d = parse_duration("1m");
        REQUIRE(d.has_value());
        REQUIRE(d->count() == 30 * 24 * 3600);
    }
}

TEST_CASE("parse_duration rejects invalid inputs", "[duration]") {
    REQUIRE_FALSE(parse_duration("abc").has_value());
    REQUIRE_FALSE(parse_duration("5h").has_value());
    REQUIRE_FALSE(parse_duration("1y").has_value());
    REQUIRE_FALSE(parse_duration("").has_value());
    REQUIRE_FALSE(parse_duration("d").has_value());
    REQUIRE_FALSE(parse_duration("-3d").has_value());
}
