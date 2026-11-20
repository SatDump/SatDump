#include "common/colormaps.h"
#include "helpers/temp_files.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
    using test_helpers::TempFile;

    uint32_t abgr(uint8_t r, uint8_t g, uint8_t b) { return 0xFF000000u | (uint32_t(b) << 16) | (uint32_t(g) << 8) | r; }
} // namespace

TEST_CASE("colormaps::loadMap reads name, author and hex colors", "[common][colormaps]")
{
    TempFile f("colormap.json", R"({"name": "Test", "author": "Unit", "map": ["#000000", "#FF8040", "#0a0B0c"]})");

    colormaps::Map map = colormaps::loadMap(f.path.string());

    CHECK(map.name == "Test");
    CHECK(map.author == "Unit");
    REQUIRE(map.entryCount == 3);
    REQUIRE(map.map != nullptr);
    float expected[9] = {0, 0, 0, 255, 128, 64, 10, 11, 12};
    for (int i = 0; i < 9; i++)
        CHECK(map.map[i] == expected[i]);

    delete[] map.map;
}

TEST_CASE("colormaps::loadMap returns an empty map when keys are missing", "[common][colormaps]")
{
    TempFile f("colormap.json", R"({"name": "Test"})");

    colormaps::Map map = colormaps::loadMap(f.path.string());

    CHECK(map.entryCount == 0);
    CHECK(map.map == nullptr);
}

TEST_CASE("colormaps::loadMap throws on invalid JSON", "[common][colormaps]")
{
    TempFile f("colormap.json", "this is not json");
    CHECK_THROWS(colormaps::loadMap(f.path.string()));
}

TEST_CASE("colormaps::generatePalette interpolates between entries", "[common][colormaps]")
{
    float colors[6] = {0, 0, 0, 255, 128, 64};
    colormaps::Map map;
    map.map = colors;
    map.entryCount = 2;

    std::vector<uint32_t> pal = colormaps::generatePalette(map, 4);

    REQUIRE(pal.size() == 4);
    CHECK(pal[0] == abgr(0, 0, 0));
    CHECK(pal[1] == abgr(127, 64, 32)); // Halfway, truncated
    CHECK(pal[2] == abgr(255, 128, 64));
    CHECK(pal[3] == abgr(255, 128, 64)); // Clamped to the last entry
}

TEST_CASE("colormaps::generatePalette with a single entry is constant", "[common][colormaps]")
{
    float colors[3] = {10, 20, 30};
    colormaps::Map map;
    map.map = colors;
    map.entryCount = 1;

    std::vector<uint32_t> pal = colormaps::generatePalette(map, 16);

    REQUIRE(pal.size() == 16);
    for (uint32_t c : pal)
        CHECK(c == abgr(10, 20, 30));

    CHECK(colormaps::generatePalette(map, 0).empty());
}
