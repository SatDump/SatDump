#include "common/map/map_drawer.h"
#include "helpers/temp_files.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string>

namespace
{
    // Equirectangular projection onto a 361 x 181 image: x = lon + 180, y = 90 - lat,
    // so every whole degree is a pixel and the far edges stay inside the image.
    std::pair<int, int> plateCarree(double lat, double lon, int height, int width) { return {(int)((lon + 180.0) / 360.0 * (width - 1)), (int)((90.0 - lat) / 180.0 * (height - 1))}; }

    bool isSet(image::Image &img, double lat, double lon)
    {
        auto p = plateCarree(lat, lon, img.height(), img.width());
        return img.get(0, p.first, p.second) != 0;
    }

    size_t countSet(image::Image &img)
    {
        size_t n = 0;
        for (size_t i = 0; i < img.width() * img.height(); i++)
            n += img.get(0, i) != 0;
        return n;
    }

    using test_helpers::TempFile;

    std::string feature(const std::string &type, const std::string &coords)
    {
        return R"({"type": "Feature", "properties": {}, "geometry": {"type": ")" + type + R"(", "coordinates": )" + coords + "}}";
    }

    std::string collection(const std::string &features) { return R"({"type": "FeatureCollection", "features": [)" + features + "]}"; }

    // Shapefile bytes for a single polyline record (see test_shapefile.cpp for the layout)
    std::string shapefilePolyline(const std::vector<std::pair<double, double>> &pts)
    {
        auto be32 = [](std::string &s, int32_t v)
        {
            for (int i = 3; i >= 0; i--)
                s.push_back((char)((uint32_t)v >> (i * 8)));
        };
        auto le32 = [](std::string &s, int32_t v)
        {
            for (int i = 0; i < 4; i++)
                s.push_back((char)((uint32_t)v >> (i * 8)));
        };
        auto le64f = [](std::string &s, double v)
        {
            uint64_t u;
            std::memcpy(&u, &v, 8);
            for (int i = 0; i < 8; i++)
                s.push_back((char)(u >> (i * 8)));
        };

        std::string c;
        for (int i = 0; i < 4; i++)
            le64f(c, 0);
        le32(c, 1);
        le32(c, (int32_t)pts.size());
        le32(c, 0);
        for (auto &p : pts)
        {
            le64f(c, p.first);
            le64f(c, p.second);
        }
        std::string rec;
        be32(rec, 1);
        be32(rec, (int32_t)(4 + c.size()) / 2);
        le32(rec, 3);
        rec += c;

        // A trailing point record so the polyline is not the last thing in the file
        std::string pc;
        le64f(pc, 50);
        le64f(pc, -40);
        be32(rec, 2);
        be32(rec, 10);
        le32(rec, 1);
        rec += pc;

        std::string h;
        be32(h, 9994);
        for (int i = 0; i < 5; i++)
            be32(h, 0);
        be32(h, (int32_t)(100 + rec.size()) / 2);
        le32(h, 1000);
        le32(h, 3);
        for (int i = 0; i < 8; i++)
            le64f(h, 0);
        return h + rec;
    }
} // namespace

TEST_CASE("map::drawProjectedMapGeoJson draws polygon edges including the closing edge", "[common][map][map_drawer]")
{
    TempFile f(".json", collection(feature("Polygon", "[[[0, 0], [20, 0], [20, 10], [0, 10], [0, 0]]]")));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapGeoJson({f.path.string()}, img, {1.0}, plateCarree);

    // GeoJSON is [lon, lat]
    CHECK(isSet(img, 0, 10));       // bottom edge
    CHECK(isSet(img, 5, 20));       // right edge
    CHECK(isSet(img, 10, 10));      // top edge
    CHECK(isSet(img, 5, 0));        // left edge
    CHECK_FALSE(isSet(img, 5, 10)); // interior stays empty
    CHECK(img.get(0, 180 + 10, 90) == 255);
}

TEST_CASE("map::drawProjectedMapGeoJson draws multipolygons", "[common][map][map_drawer]")
{
    TempFile f(".json", collection(feature("MultiPolygon", "[[[[0, 0], [10, 0], [10, 10], [0, 0]]], [[[100, -20], [110, -20], [110, -30], [100, -20]]]]")));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapGeoJson({f.path.string()}, img, {1.0}, plateCarree);
    CHECK(isSet(img, 0, 5));
    CHECK(isSet(img, -20, 105));
    CHECK(isSet(img, -25, 110));
}

TEST_CASE("map::drawProjectedMapGeoJson draws points and ignores non-features", "[common][map][map_drawer]")
{
    TempFile f(".json", collection(feature("Point", "[-74, 40]") + "," + R"({"type": "NotAFeature", "geometry": {"type": "Point", "coordinates": [10, 10]}})"));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapGeoJson({f.path.string()}, img, {1.0}, plateCarree);
    CHECK(isSet(img, 40, -74));
    CHECK_FALSE(isSet(img, 10, 10));
    CHECK(countSet(img) == 1);
}

TEST_CASE("map::drawProjectedMapGeoJson skips segments longer than maxLength", "[common][map][map_drawer]")
{
    TempFile f(".json", collection(feature("LineString", "[[0, 0], [100, 0]]") + "," + feature("LineString", "[[0, 20], [5, 20]]")));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapGeoJson({f.path.string()}, img, {1.0}, plateCarree, 50);
    CHECK_FALSE(isSet(img, 0, 50));
    CHECK(isSet(img, 20, 2));
}

TEST_CASE("map::drawProjectedMapGeoJson skips points the projection rejects", "[common][map][map_drawer]")
{
    TempFile f(".json", collection(feature("LineString", "[[0, 0], [10, 0]]")));
    image::Image img(8, 361, 181, 1);
    auto rejectAll = [](double, double, int, int) { return std::pair<int, int>(-1, -1); };
    map::drawProjectedMapGeoJson({f.path.string()}, img, {1.0}, rejectAll);
    CHECK(countSet(img) == 0);
}

// Bug: map_drawer.cpp:127-140 always draws a segment from the last LineString vertex back to the
// first, which turns open lines (rivers, borders) into closed loops.
TEST_CASE("map::drawProjectedMapGeoJson does not close LineStrings", "[common][map][map_drawer][!shouldfail]")
{
    TempFile f(".json", collection(feature("LineString", "[[0, 0], [20, 0], [20, 20]]")));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapGeoJson({f.path.string()}, img, {1.0}, plateCarree);
    CHECK(isSet(img, 0, 10));
    CHECK(isSet(img, 10, 20));
    CHECK_FALSE(isSet(img, 10, 10)); // on the diagonal from the last vertex back to the first
}

TEST_CASE("map::drawProjectedMapShapefile draws point records", "[common][map][map_drawer]")
{
    TempFile f(".shp", shapefilePolyline({{0, 0}, {20, 0}, {20, 20}}));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapShapefile({f.path.string()}, img, {1.0}, plateCarree);
    CHECK(isSet(img, -40, 50));
}

// Bug: Two off-by-ones drop the end of every polyline: shapefile.cpp:120 loses the last vertex when
// parsing, and the drawing loop at map_drawer.cpp:214 stops at size() - 1. A 3-vertex line ends
// up drawing nothing at all.
TEST_CASE("map::drawProjectedMapShapefile draws every polyline segment", "[common][map][map_drawer][!shouldfail]")
{
    TempFile f(".shp", shapefilePolyline({{0, 0}, {20, 0}, {20, 20}}));
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapShapefile({f.path.string()}, img, {1.0}, plateCarree);
    CHECK(isSet(img, 0, 10));
    CHECK(isSet(img, 10, 20));
}

TEST_CASE("map::drawProjectedMapLatLonGrid draws a 10 degree graticule", "[common][map][map_drawer]")
{
    image::Image img(8, 361, 181, 1);
    map::drawProjectedMapLatLonGrid(img, {1.0}, plateCarree);
    // Meridians every 10 degrees of longitude
    CHECK(isSet(img, 45, 0));
    CHECK(isSet(img, -33, -170));
    CHECK(isSet(img, 12, 90));
    // Parallels every 10 degrees of latitude
    CHECK(isSet(img, 30, 15));
    CHECK(isSet(img, -60, -123));
    // Between grid lines nothing is drawn
    CHECK_FALSE(isSet(img, 45, 5));
    CHECK_FALSE(isSet(img, -15, -125));
}

TEST_CASE("map::drawProjectedCitiesGeoJson does nothing without a font", "[common][map][map_drawer]")
{
    image::Image img(8, 361, 181, 1);
    image::TextDrawer text;
    // The file is never opened when no font is loaded
    map::drawProjectedCitiesGeoJson({"/nonexistent/satdump_cities.json"}, img, text, {1.0}, plateCarree);
    CHECK(countSet(img) == 0);
}
