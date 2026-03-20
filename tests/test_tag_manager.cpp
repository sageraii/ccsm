#include <catch2/catch_test_macros.hpp>
#include "core/tag_manager.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>

using namespace ccsm;
namespace fs = std::filesystem;

static const std::string SID = "aaaa1111-0000-0000-0000-000000000001";
static const std::string SID2 = "bbbb2222-0000-0000-0000-000000000002";

class TagManagerFixture {
public:
    fs::path tmp_path;
    TagManagerFixture() {
        tmp_path = fs::temp_directory_path() / "ccsm_test_tags.json";
        fs::copy_file(fs::path("fixtures/tags_sample.json"), tmp_path,
                      fs::copy_options::overwrite_existing);
    }
    ~TagManagerFixture() { fs::remove(tmp_path); }
};

TEST_CASE("TagManager loads existing tags", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    auto data = mgr.get(SID);
    REQUIRE(data.has_value());
    REQUIRE(data->tags.size() == 2);
    REQUIRE(data->favorite);
    REQUIRE(data->note == "IDM pipeline");
}

TEST_CASE("TagManager add/remove tag", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);

    mgr.add_tag(SID, "newTag");
    auto data = mgr.get(SID);
    REQUIRE(std::find(data->tags.begin(), data->tags.end(), "newTag") != data->tags.end());

    mgr.remove_tag(SID, "newTag");
    data = mgr.get(SID);
    REQUIRE(std::find(data->tags.begin(), data->tags.end(), "newTag") == data->tags.end());
}

TEST_CASE("TagManager add_tag is idempotent", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    mgr.add_tag(SID, "training"); // already exists
    auto data = mgr.get(SID);
    auto count = std::count(data->tags.begin(), data->tags.end(), "training");
    REQUIRE(count == 1); // not duplicated
}

TEST_CASE("TagManager set note", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    mgr.set_note(SID2, "new note");
    auto data = mgr.get(SID2);
    REQUIRE(data.has_value());
    REQUIRE(data->note == "new note");
}

TEST_CASE("TagManager toggle favorite", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    REQUIRE(mgr.get(SID)->favorite == true);
    mgr.toggle_favorite(SID);
    REQUIRE(mgr.get(SID)->favorite == false);
    mgr.toggle_favorite(SID);
    REQUIRE(mgr.get(SID)->favorite == true);
}

TEST_CASE("TagManager save persists to disk", "[tags]") {
    TagManagerFixture f;
    {
        TagManager mgr(f.tmp_path);
        mgr.add_tag(SID2, "persist");
        mgr.save();
    }
    TagManager mgr2(f.tmp_path);
    auto data = mgr2.get(SID2);
    REQUIRE(data.has_value());
    REQUIRE(std::find(data->tags.begin(), data->tags.end(), "persist") != data->tags.end());
}

TEST_CASE("TagManager handles nonexistent file", "[tags]") {
    auto tmp = fs::temp_directory_path() / "ccsm_new_tags.json";
    fs::remove(tmp);
    TagManager mgr(tmp);
    mgr.add_tag(SID, "first");
    mgr.save();
    REQUIRE(fs::exists(tmp));
    fs::remove(tmp);
}

TEST_CASE("TagManager get_favorites", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    auto favs = mgr.get_favorites();
    REQUIRE(favs.size() == 1);
    REQUIRE(favs[0] == SID);
}

TEST_CASE("TagManager cleanup keeps active sessions", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    mgr.cleanup({SID}, 0); // SID is active, max_age=0
    REQUIRE(mgr.get(SID).has_value()); // kept because active
}

TEST_CASE("TagManager cleanup removes old expired sessions", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    // SID is NOT in active list, lastSeen is old, max_age_days=0
    mgr.cleanup({}, 0);
    REQUIRE_FALSE(mgr.get(SID).has_value()); // removed
}

TEST_CASE("TagManager cleanup respects max_age_days", "[tags]") {
    TagManagerFixture f;
    TagManager mgr(f.tmp_path);
    // SID not active, but max_age is huge (9999 days) so it stays
    mgr.cleanup({}, 9999);
    REQUIRE(mgr.get(SID).has_value()); // kept — within age limit
}
