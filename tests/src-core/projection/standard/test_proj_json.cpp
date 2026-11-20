#include "projection/standard/proj_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("proj from_json parses every projection type name", "[projection][standard][proj_json]")
{
    struct Case
    {
        const char *name;
        proj::projection_type_t type;
    };
    const Case cases[] = {
        {"equirec", proj::ProjType_Equirectangular},
        {"stereo", proj::ProjType_Stereographic},
        {"utm", proj::ProjType_UniversalTransverseMercator},
        {"geos", proj::ProjType_Geos},
        {"tpers", proj::ProjType_Tpers},
        {"webmerc", proj::ProjType_WebMerc},
        {"not_a_projection", proj::ProjType_Invalid},
    };
    for (const auto &c : cases)
    {
        proj::projection_t p = nlohmann::json{{"type", c.name}}.get<proj::projection_t>();
        CHECK(p.type == c.type);
    }
}

TEST_CASE("proj from_json without a type throws", "[projection][standard][proj_json]") { CHECK_THROWS(nlohmann::json{{"lon0", 3.0}}.get<proj::projection_t>()); }

TEST_CASE("proj from_json reads offsets, scalars and the center in degrees", "[projection][standard][proj_json]")
{
    nlohmann::json j = {{"type", "equirec"}, {"offset_x", -10.5}, {"offset_y", 20.25}, {"scalar_x", 0.5}, {"scalar_y", -0.25}, {"lon0", 90.0}, {"lat0", -45.0}};
    proj::projection_t p = j.get<proj::projection_t>();
    CHECK(p.proj_offset_x == -10.5);
    CHECK(p.proj_offset_y == 20.25);
    CHECK(p.proj_scalar_x == 0.5);
    CHECK(p.proj_scalar_y == -0.25);
    CHECK_THAT(p.lam0, WithinAbs(M_PI / 2, 1e-15));
    CHECK_THAT(p.phi0, WithinAbs(-M_PI / 4, 1e-15));
}

TEST_CASE("proj from_json defaults leave an identity pixel transform", "[projection][standard][proj_json]")
{
    proj::projection_t p = nlohmann::json{{"type", "geos"}}.get<proj::projection_t>();
    CHECK(p.proj_offset_x == 0);
    CHECK(p.proj_offset_y == 0);
    CHECK(p.proj_scalar_x == 1);
    CHECK(p.proj_scalar_y == 1);
    CHECK(p.lam0 == 0);
    CHECK(p.phi0 == 0);
    CHECK(p.params.altitude == 0);
    CHECK_FALSE(p.params.sweep_x);
}

TEST_CASE("proj to_json/from_json round-trips type-specific parameters", "[projection][standard][proj_json]")
{
    SECTION("utm")
    {
        proj::projection_t p = nlohmann::json{{"type", "utm"}, {"zone", 12}, {"south", true}, {"offset_y", 3.0}}.get<proj::projection_t>();
        nlohmann::json j = p;
        CHECK(j["type"] == "utm");
        CHECK(j["zone"] == 12);
        CHECK(j["south"] == true);
        CHECK(j["offset_y"] == 3.0);
        CHECK_FALSE(j.contains("offset_x")); // Defaults are not written
        CHECK_FALSE(j.contains("scalar_x"));
        proj::projection_t p2 = j.get<proj::projection_t>();
        CHECK(p2.params.zone == 12);
        CHECK(p2.params.south);
    }

    SECTION("geos")
    {
        proj::projection_t p = nlohmann::json{{"type", "geos"}, {"altitude", 35786023.0}, {"sweep_x", true}, {"lon0", -137.2}}.get<proj::projection_t>();
        nlohmann::json j = p;
        CHECK(j["type"] == "geos");
        CHECK(j["altitude"] == 35786023.0);
        CHECK(j["sweep_x"] == true);
        CHECK_THAT(j["lon0"].get<double>(), WithinAbs(-137.2, 1e-12));
        CHECK_FALSE(j.contains("lat0"));
        proj::projection_t p2 = j.get<proj::projection_t>();
        CHECK(p2.params.altitude == 35786023.0);
        CHECK(p2.params.sweep_x);
        CHECK_THAT(p2.lam0, WithinAbs(p.lam0, 1e-15));
    }

    SECTION("tpers")
    {
        proj::projection_t p = nlohmann::json{{"type", "tpers"}, {"altitude", 1e7}, {"tilt", 12.0}, {"azimuth", 34.0}, {"lat0", 45.0}, {"scalar_x", 7.0}}.get<proj::projection_t>();
        nlohmann::json j = p;
        CHECK(j["type"] == "tpers");
        CHECK(j["altitude"] == 1e7);
        CHECK(j["tilt"] == 12.0);
        CHECK(j["azimuth"] == 34.0);
        CHECK(j["scalar_x"] == 7.0);
        CHECK_THAT(j["lat0"].get<double>(), WithinAbs(45, 1e-12));
        proj::projection_t p2 = j.get<proj::projection_t>();
        CHECK(p2.params.tilt == 12.0);
        CHECK(p2.params.azimuth == 34.0);
    }

    SECTION("equirec, stereo and webmerc carry no extra parameters")
    {
        for (const char *t : {"equirec", "stereo", "webmerc"})
        {
            nlohmann::json j = nlohmann::json{{"type", t}}.get<proj::projection_t>();
            CHECK(j == nlohmann::json{{"type", t}});
        }
    }
}

TEST_CASE("proj from_json expands stereo and tpers image configs", "[projection][standard][proj_json]")
{
    for (const char *t : {"stereo", "tpers"})
    {
        nlohmann::json j = {{"type", t}, {"center_lon", 12.0}, {"center_lat", -34.0}, {"scale", 3000.0}, {"width", 640}, {"height", 480}};
        proj::projection_t p = j.get<proj::projection_t>();
        CHECK_THAT(p.lam0 * 180 / M_PI, WithinAbs(12, 1e-12));
        CHECK_THAT(p.phi0 * 180 / M_PI, WithinAbs(-34, 1e-12));
        CHECK(p.proj_scalar_x == 3000.0);
        CHECK(p.proj_scalar_y == -3000.0);
        CHECK(p.proj_offset_x == -640 * 0.5 * 3000.0);
        CHECK(p.proj_offset_y == 480 * 0.5 * 3000.0);
    }

    // Without width/height the shortcut keys are ignored
    proj::projection_t p = nlohmann::json{{"type", "stereo"}, {"center_lon", 12.0}, {"center_lat", -34.0}, {"scale", 3000.0}}.get<proj::projection_t>();
    CHECK(p.lam0 == 0);
    CHECK(p.proj_scalar_x == 1);
}

TEST_CASE("proj from_json expands a UTM image config keeping the northing offset", "[projection][standard][proj_json]")
{
    nlohmann::json j = {{"type", "utm"}, {"scale", 500.0}, {"width", 100}, {"height", 200}, {"offset_y", 4000000.0}};
    proj::projection_t p = j.get<proj::projection_t>();
    CHECK(p.proj_scalar_x == 500.0);
    CHECK(p.proj_scalar_y == -500.0);
    CHECK(p.proj_offset_x == -100 * 0.5 * 500.0);
    CHECK(p.proj_offset_y == 200 * 0.5 * 500.0 + 4000000.0);
}
