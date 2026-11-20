#include "db/db_handler.h"
#include "handlers/vector/shapefile_handler.h"
#include "init.h"
#include "pipeline/pipeline_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test;
using satdump::handlers::ShapefileHandler;

namespace
{
    // ShapefileHandler persists its settings in the user database, so give it a temporary one
    struct TempDatabase
    {
        TempDir dir;
        std::shared_ptr<satdump::DBHandler> saved;

        TempDatabase() : dir("handlers_shp_db"), saved(satdump::db)
        {
            satdump::db = std::make_shared<satdump::DBHandler>(dir.file("test.db"));
            satdump::db->init();
        }

        ~TempDatabase() { satdump::db = saved; }
    };

    const std::string PLACES = std::string(SATDUMP_TEST_SOURCE_DIR) + "/resources/maps/ne_10m_populated_places_simple.shp";
} // namespace

TEST_CASE("ShapefileHandler default configuration", "[handlers][shapefile_handler]")
{
    TempDatabase db;
    ShapefileHandler h;

    CHECK(h.getID() == "shapefile_handler");
    CHECK(h.getName() == "");
    CHECK_FALSE(h.has_dbf);
    CHECK(h.file == nullptr);

    auto cfg = h.getConfig();
    CHECK(cfg["color"] == nlohmann::json{0.0, 1.0, 0.0, 1.0});
    CHECK(cfg["antialias"] == false);
    CHECK(cfg["font_size"] == 20);
    CHECK(cfg["scalerank_filter"] == 3);
}

TEST_CASE("ShapefileHandler::setConfig updates only the given keys", "[handlers][shapefile_handler]")
{
    TempDatabase db;
    ShapefileHandler h;

    h.setConfig({{"font_size", 32}});
    CHECK(h.font_size == 32);
    CHECK(h.antialias == false);
    CHECK(h.scalerank_filter == 3);

    h.setConfig({{"color", {0.25, 0.5, 0.75, 1.0}}, {"antialias", true}, {"scalerank_filter", 7}});
    CHECK(h.color_to_draw.x == Catch::Approx(0.25));
    CHECK(h.color_to_draw.y == Catch::Approx(0.5));
    CHECK(h.color_to_draw.z == Catch::Approx(0.75));
    CHECK(h.color_to_draw.w == Catch::Approx(1.0));
    CHECK(h.antialias);
    CHECK(h.scalerank_filter == 7);

    // Round trip
    ShapefileHandler h2;
    h2.setConfig(h.getConfig());
    CHECK(h2.getConfig() == h.getConfig());

    // Null (as returned by the database for unknown entries) is ignored
    h2.setConfig(nlohmann::json());
    CHECK(h2.font_size == 32);
}

TEST_CASE("ShapefileHandler loads a shapefile and its attribute table", "[handlers][shapefile_handler]")
{
    TempDatabase db;
    REQUIRE(std::filesystem::exists(PLACES));

    ShapefileHandler h(PLACES);
    CHECK(h.getName() == "ne_10m_populated_places_simple.shp");
    REQUIRE(h.file != nullptr);
    CHECK(h.file->point_records.size() > 1000);

    REQUIRE(h.has_dbf);
    REQUIRE(h.dbf_file.is_array());
    // One attribute row per point
    CHECK(h.dbf_file.size() == h.file->point_records.size());
    CHECK(h.dbf_file[0]["name"].is_string());
}

TEST_CASE("ShapefileHandler remembers its settings per shapefile", "[handlers][shapefile_handler]")
{
    TempDatabase db;

    {
        ShapefileHandler h(PLACES);
        CHECK(h.font_size == 20);
        h.setConfig({{"font_size", 44}, {"antialias", true}});
    } // saved on destruction

    CHECK(satdump::db->get_user_json("shapefile_defaults/ne_10m_populated_places_simple")["font_size"] == 44);

    ShapefileHandler again(PLACES);
    CHECK(again.font_size == 44);
    CHECK(again.antialias);
}
