#include "image/geotiff/geotiff.h"
#include "image/image_test_helpers.h"
#include "image/io.h"
#include "image/meta.h"
#include "projection/standard/proj_json.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <tiffio.h>

using Catch::Approx;
using satdump::image::Image;
namespace img = satdump::image;

namespace
{
    // Saves a small image with the given proj_cfg as TIFF and loads it back
    nlohmann::json tiffProjRoundTrip(const nlohmann::json &proj_cfg, bool &has_proj)
    {
        image_test::TempDir dir;
        std::string path = dir.file("geo.tif");
        Image in = image_test::makePattern(16, 8, 6, 1);
        img::set_metadata_proj_cfg(in, proj_cfg);
        img::save_tiff(in, path);

        Image out;
        img::load_tiff(out, path);
        REQUIRE(image_test::samePixels(out, in));
        has_proj = img::has_metadata_proj_cfg(out);
        return img::get_metadata_proj_cfg(out);
    }
} // namespace

TEST_CASE("geotiff equirectangular projection round-trips through TIFF", "[image][geotiff]")
{
    bool has_proj = false;
    auto cfg = tiffProjRoundTrip({{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", 0.5}, {"scalar_y", -0.25}}, has_proj);
    REQUIRE(has_proj);
    CHECK(cfg["type"] == "equirec");
    CHECK(cfg["offset_x"].get<double>() == Approx(-180.0));
    CHECK(cfg["offset_y"].get<double>() == Approx(90.0));
    CHECK(cfg["scalar_x"].get<double>() == Approx(0.5));
    CHECK(cfg["scalar_y"].get<double>() == Approx(-0.25));
    // The loader adds the image size
    CHECK(cfg["width"] == 8);
    CHECK(cfg["height"] == 6);
}

TEST_CASE("geotiff stereographic projection keeps its centre", "[image][geotiff]")
{
    bool has_proj = false;
    auto cfg = tiffProjRoundTrip({{"type", "stereo"}, {"lon0", 12.5}, {"lat0", 45.0}, {"offset_x", -1000.0}, {"offset_y", 2000.0}, {"scalar_x", 250.0}, {"scalar_y", -250.0}}, has_proj);
    REQUIRE(has_proj);
    CHECK(cfg["type"] == "stereo");
    CHECK(cfg["lon0"].get<double>() == Approx(12.5));
    CHECK(cfg["lat0"].get<double>() == Approx(45.0));
    CHECK(cfg["offset_x"].get<double>() == Approx(-1000.0));
    CHECK(cfg["scalar_y"].get<double>() == Approx(-250.0));
}

TEST_CASE("geotiff UTM zone and hemisphere round-trip", "[image][geotiff]")
{
    for (bool south : {false, true})
    {
        INFO("south " << south);
        bool has_proj = false;
        auto cfg = tiffProjRoundTrip({{"type", "utm"}, {"zone", 33}, {"south", south}, {"offset_x", 500000.0}, {"offset_y", 1000.0}, {"scalar_x", 30.0}, {"scalar_y", -30.0}}, has_proj);
        REQUIRE(has_proj);
        CHECK(cfg["type"] == "utm");
        CHECK(cfg["zone"] == 33);
        CHECK(cfg["south"] == south);
        CHECK(cfg["offset_x"].get<double>() == Approx(500000.0));
        CHECK(cfg["scalar_x"].get<double>() == Approx(30.0));
    }
}

TEST_CASE("geotiff web mercator round-trips", "[image][geotiff]")
{
    bool has_proj = false;
    auto cfg = tiffProjRoundTrip({{"type", "webmerc"}, {"offset_x", -20037508.0}, {"offset_y", 20037508.0}, {"scalar_x", 1000.0}, {"scalar_y", -1000.0}}, has_proj);
    REQUIRE(has_proj);
    CHECK(cfg["type"] == "webmerc");
    CHECK(cfg["offset_y"].get<double>() == Approx(20037508.0));
}

TEST_CASE("geotiff unsupported projections are not written", "[image][geotiff]")
{
    bool has_proj = true;
    tiffProjRoundTrip({{"type", "geos"}, {"altitude", 35786023.0}}, has_proj);
    CHECK_FALSE(has_proj);

    // Invalid proj_cfg (no type) is ignored as well
    has_proj = true;
    tiffProjRoundTrip({{"something", 1}}, has_proj);
    CHECK_FALSE(has_proj);
}

TEST_CASE("geotiff try_write_geotiff rejects invalid and unsupported projections", "[image][geotiff]")
{
    image_test::TempDir dir;
    std::string path = dir.file("direct.tif");
    TIFF *tif = TIFFOpen(path.c_str(), "w");
    REQUIRE(tif != nullptr);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

    proj::projection_t invalid;
    CHECK(satdump::geotiff::try_write_geotiff(tif, &invalid));

    proj::projection_t geos;
    geos.type = proj::ProjType_Geos;
    CHECK(satdump::geotiff::try_write_geotiff(tif, &geos));

    proj::projection_t tpers;
    tpers.type = proj::ProjType_Tpers;
    CHECK(satdump::geotiff::try_write_geotiff(tif, &tpers));

    proj::projection_t eq;
    eq.type = proj::ProjType_Equirectangular;
    CHECK_FALSE(satdump::geotiff::try_write_geotiff(tif, &eq));
    uint8_t px = 0;
    TIFFWriteScanline(tif, &px, 0, 0);
    TIFFClose(tif);

    // The tags written are enough for the reader to recover the projection
    tif = TIFFOpen(path.c_str(), "r");
    REQUIRE(tif != nullptr);
    proj::projection_t back;
    CHECK(satdump::geotiff::try_read_geotiff(&back, tif));
    CHECK(back.type == proj::ProjType_Equirectangular);
    TIFFClose(tif);
}

TEST_CASE("geotiff try_read_geotiff on a plain TIFF fails", "[image][geotiff]")
{
    image_test::TempDir dir;
    std::string path = dir.file("plain.tif");
    Image in(8, 4, 4, 1);
    img::save_tiff(in, path);

    TIFF *tif = TIFFOpen(path.c_str(), "r");
    REQUIRE(tif != nullptr);
    proj::projection_t p;
    p.type = proj::ProjType_Stereographic;
    CHECK_FALSE(satdump::geotiff::try_read_geotiff(&p, tif));
    // The projection is reset even on failure
    CHECK(p.type == proj::ProjType_Invalid);
    TIFFClose(tif);
}
