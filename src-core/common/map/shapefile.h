#pragma once

#include <cstdint>
#include <fstream>
#include <vector>

/*
Basic Shapefile parser, based on https://support.esri.com/en/white-paper/279.
This does not implement the full specification, only NullShape, Point, MultiPoint, 
PolyLine and Polygon are currently implemented.
*/

namespace shapefile
{
    enum ShapeType
    {
        NullShape = 0,
        Point = 1,
        PolyLine = 3,
        Polygon = 5,
        MultiPoint = 8,
        PointZ = 11,      // Unsupported
        PolyLineZ = 13,   // Unsupported
        PolygonZ = 15,    // Unsupported
        MultiPointZ = 18, // Unsupported
        PointM = 21,      // Unsupported
        PolyLineM = 23,   // Unsupported
        PolygonM = 25,    // Unsupported
        MultiPointM = 28, // Unsupported
        MultiPatch = 31   // Unsupported
    };

#ifdef _WIN32
#pragma pack(push, 1)
#endif
    struct box_t
    {
        double box1;
        double box2;
        double box3;
        double box4;
    }
#ifdef _WIN32
    ;
#else
    __attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
    struct point_t
    {
        double x;
        double y;
    }
#ifdef _WIN32
    ;
#else
    __attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

    struct ShapefileHeader
    {
        int32_t file_code;
        int32_t unused1;
        int32_t unused2;
        int32_t unused3;
        int32_t unused4;
        int32_t unused5;
        int32_t file_length;

        int32_t version;
        int32_t shape_type;

        double bounding_box_xmin;
        double bounding_box_ymin;
        double bounding_box_xmax;
        double bounding_box_ymax;
        double bounding_box_zmin;
        double bounding_box_zmax;
        double bounding_box_mmin;
        double bounding_box_mmax;

        ShapefileHeader(std::istream &stream);
    };

    struct RecordHeader
    {
        int32_t record_number;
        int32_t content_length;
        int32_t shape_type;

        RecordHeader(std::istream &stream);
    };

    struct NullShapeRecord : RecordHeader
    {
        NullShapeRecord(std::istream &stream, RecordHeader &header);
    };

    struct PointRecord : RecordHeader
    {
        point_t point;

        PointRecord(std::istream &stream, RecordHeader &header);
    };

    struct MultiPointRecord : RecordHeader
    {
        box_t box;
        int32_t point_number;
        std::vector<point_t> points;

        MultiPointRecord(std::istream &stream, RecordHeader &header);
    };

    struct PolyLineRecord : RecordHeader
    {
        box_t box;
        int32_t part_number;
        int32_t point_number;
        std::vector<std::vector<point_t>> parts_points;

        PolyLineRecord(std::istream &stream, RecordHeader &header);
    };

    struct PolygonRecord : public PolyLineRecord // Identical
    {
        PolygonRecord(std::istream &stream, RecordHeader &header);
    };

    struct Shapefile
    {
        ShapefileHeader header;

        std::vector<PointRecord> point_records;
        std::vector<MultiPointRecord> multipoint_records;
        std::vector<PolyLineRecord> polyline_records;
        std::vector<PolygonRecord> polygon_records;

        Shapefile(std::istream &stream);
    };
};