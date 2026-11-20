#include "common/map/shapefile.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // Minimal ESRI shapefile writer: file/record headers are big-endian, contents little-endian
    struct ShpWriter
    {
        std::string body;

        void be32(std::string &s, int32_t v)
        {
            for (int i = 3; i >= 0; i--)
                s.push_back((char)((uint32_t)v >> (i * 8)));
        }
        void le32(std::string &s, int32_t v)
        {
            for (int i = 0; i < 4; i++)
                s.push_back((char)((uint32_t)v >> (i * 8)));
        }
        void le64f(std::string &s, double v)
        {
            uint64_t u;
            std::memcpy(&u, &v, 8);
            for (int i = 0; i < 8; i++)
                s.push_back((char)(u >> (i * 8)));
        }

        int record_number = 1;

        void record(int32_t type, const std::string &content)
        {
            std::string r;
            be32(r, record_number++);
            be32(r, (int32_t)(4 + content.size()) / 2); // Length in 16-bit words, including the shape type
            le32(r, type);
            body += r + content;
        }

        void point(double x, double y)
        {
            std::string c;
            le64f(c, x);
            le64f(c, y);
            record(shapefile::Point, c);
        }

        void nullShape() { record(shapefile::NullShape, ""); }

        void box(std::string &c, double x0, double y0, double x1, double y1)
        {
            le64f(c, x0);
            le64f(c, y0);
            le64f(c, x1);
            le64f(c, y1);
        }

        void multiPoint(const std::vector<std::pair<double, double>> &pts)
        {
            std::string c;
            box(c, -1, -2, 3, 4);
            le32(c, (int32_t)pts.size());
            for (auto &p : pts)
            {
                le64f(c, p.first);
                le64f(c, p.second);
            }
            record(shapefile::MultiPoint, c);
        }

        void poly(int32_t type, const std::vector<std::vector<std::pair<double, double>>> &parts)
        {
            std::string c;
            box(c, -10, -20, 30, 40);
            int total = 0;
            for (auto &p : parts)
                total += p.size();
            le32(c, (int32_t)parts.size());
            le32(c, total);
            int start = 0;
            for (auto &p : parts)
            {
                le32(c, start);
                start += p.size();
            }
            for (auto &p : parts)
                for (auto &pt : p)
                {
                    le64f(c, pt.first);
                    le64f(c, pt.second);
                }
            record(type, c);
        }

        void unsupported(int32_t type, int bytes) { record(type, std::string(bytes, '\0')); }

        std::string file(int32_t shape_type)
        {
            std::string h;
            be32(h, 9994);
            for (int i = 0; i < 5; i++)
                be32(h, 0);
            be32(h, (int32_t)(100 + body.size()) / 2);
            le32(h, 1000);
            le32(h, shape_type);
            for (double v : {-180.0, -90.0, 180.0, 90.0, 0.0, 0.0, 0.0, 0.0})
                le64f(h, v);
            return h + body;
        }
    };
} // namespace

TEST_CASE("shapefile::ShapefileHeader parses the mixed-endian header", "[common][map][shapefile]")
{
    ShpWriter w;
    std::istringstream in(w.file(shapefile::PolyLine));
    shapefile::ShapefileHeader h(in);
    CHECK(h.file_code == 9994);
    CHECK(h.unused1 == 0);
    // Stored in 16-bit words, exposed in bytes
    CHECK(h.file_length == 100);
    CHECK(h.version == 1000);
    CHECK(h.shape_type == shapefile::PolyLine);
    CHECK(h.bounding_box_xmin == -180.0);
    CHECK(h.bounding_box_ymin == -90.0);
    CHECK(h.bounding_box_xmax == 180.0);
    CHECK(h.bounding_box_ymax == 90.0);
}

TEST_CASE("shapefile::Shapefile with no records is empty", "[common][map][shapefile]")
{
    ShpWriter w;
    std::istringstream in(w.file(shapefile::Point));
    shapefile::Shapefile shp(in);
    CHECK(shp.header.file_code == 9994);
    CHECK(shp.point_records.empty());
    CHECK(shp.multipoint_records.empty());
    CHECK(shp.polyline_records.empty());
    CHECK(shp.polygon_records.empty());
}

TEST_CASE("shapefile::Shapefile parses point records", "[common][map][shapefile]")
{
    ShpWriter w;
    w.point(2.35, 48.85);
    w.point(-74.0, 40.7);
    std::istringstream in(w.file(shapefile::Point));
    shapefile::Shapefile shp(in);
    REQUIRE(shp.point_records.size() == 2);
    CHECK(shp.point_records[0].record_number == 1);
    CHECK(shp.point_records[0].shape_type == shapefile::Point);
    CHECK(shp.point_records[0].content_length == 16);
    CHECK(shp.point_records[0].point.x == 2.35);
    CHECK(shp.point_records[0].point.y == 48.85);
    CHECK(shp.point_records[1].record_number == 2);
    CHECK(shp.point_records[1].point.x == -74.0);
    CHECK(shp.point_records[1].point.y == 40.7);
}

TEST_CASE("shapefile::Shapefile parses multipoint records", "[common][map][shapefile]")
{
    ShpWriter w;
    w.multiPoint({{1, 2}, {3, 4}, {-5, 6.5}});
    std::istringstream in(w.file(shapefile::MultiPoint));
    shapefile::Shapefile shp(in);
    REQUIRE(shp.multipoint_records.size() == 1);
    auto &r = shp.multipoint_records[0];
    CHECK(r.box.box1 == -1);
    CHECK(r.box.box4 == 4);
    CHECK(r.point_number == 3);
    REQUIRE(r.points.size() == 3);
    CHECK(r.points[0].x == 1);
    CHECK(r.points[0].y == 2);
    CHECK(r.points[2].x == -5);
    CHECK(r.points[2].y == 6.5);
}

TEST_CASE("shapefile::Shapefile splits polyline parts and keeps the header fields", "[common][map][shapefile]")
{
    ShpWriter w;
    w.poly(shapefile::PolyLine, {{{0, 0}, {1, 1}}, {{2, 2}, {3, 3}, {4, 4}}});
    std::istringstream in(w.file(shapefile::PolyLine));
    shapefile::Shapefile shp(in);
    REQUIRE(shp.polyline_records.size() == 1);
    auto &r = shp.polyline_records[0];
    CHECK(r.box.box1 == -10);
    CHECK(r.box.box2 == -20);
    CHECK(r.box.box3 == 30);
    CHECK(r.box.box4 == 40);
    CHECK(r.part_number == 2);
    CHECK(r.point_number == 5);
    REQUIRE(r.parts_points.size() == 2);
    // Non-final parts are complete
    REQUIRE(r.parts_points[0].size() == 2);
    CHECK(r.parts_points[0][0].x == 0);
    CHECK(r.parts_points[0][1].x == 1);
    CHECK(r.parts_points[1][0].x == 2);
}

// Bug: shapefile.cpp:120 copies the last part with &points[points.size() - 1] as the end iterator,
// which drops the final point of every record (the closing vertex of polygons, the last vertex
// of polylines).
TEST_CASE("shapefile::Shapefile keeps every point of the last part", "[common][map][shapefile][!shouldfail]")
{
    ShpWriter w;
    w.poly(shapefile::PolyLine, {{{0, 0}, {1, 1}}, {{2, 2}, {3, 3}, {4, 4}}});
    w.poly(shapefile::Polygon, {{{0, 0}, {1, 0}, {1, 1}, {0, 0}}});
    std::istringstream in(w.file(shapefile::PolyLine));
    shapefile::Shapefile shp(in);
    REQUIRE(shp.polyline_records.size() == 1);
    REQUIRE(shp.polygon_records.size() == 1);
    CHECK(shp.polyline_records[0].parts_points[1].size() == 3);
    CHECK(shp.polygon_records[0].parts_points[0].size() == 4);
}

TEST_CASE("shapefile::Shapefile routes polygons, skips null and unsupported shapes", "[common][map][shapefile]")
{
    ShpWriter w;
    w.nullShape();
    w.poly(shapefile::Polygon, {{{0, 0}, {1, 0}, {1, 1}, {0, 0}}});
    w.unsupported(shapefile::PointZ, 32);
    w.point(5, 6);
    std::istringstream in(w.file(shapefile::Polygon));
    shapefile::Shapefile shp(in);
    CHECK(shp.polygon_records.size() == 1);
    CHECK(shp.polyline_records.empty());
    CHECK(shp.multipoint_records.empty());
    // The point after the skipped records is still found, so the stream stayed in sync
    REQUIRE(shp.point_records.size() == 1);
    CHECK(shp.point_records[0].record_number == 4);
    CHECK(shp.point_records[0].point.x == 5);
    CHECK(shp.point_records[0].point.y == 6);
}

TEST_CASE("shapefile::PointRecord rejects a wrong content length", "[common][map][shapefile]")
{
    ShpWriter w;
    w.unsupported(shapefile::Point, 24); // Point record with 24 bytes instead of 16
    std::istringstream in(w.file(shapefile::Point));
    CHECK_THROWS_AS(shapefile::Shapefile(in), std::runtime_error);
}
