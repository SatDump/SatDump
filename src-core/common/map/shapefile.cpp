#include "shapefile.h"
#include "utils/binary.h"
#include <string>

namespace shapefile
{
    ShapefileHeader::ShapefileHeader(std::istream &stream)
    {
        uint8_t buffer[100];
        stream.read((char *)buffer, 100);

        // Little endian
        file_code = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[0]));
        unused1 = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[4]));
        unused2 = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[8]));
        unused3 = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[12]));
        unused4 = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[16]));
        unused5 = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[20]));
        file_length = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[24])) * 2;

        // Big endian
        version = *((int32_t *)&buffer[28]);
        shape_type = *((int32_t *)&buffer[32]);

        bounding_box_xmin = *((double *)&buffer[36]);
        bounding_box_ymin = *((double *)&buffer[44]);
        bounding_box_xmax = *((double *)&buffer[52]);
        bounding_box_ymax = *((double *)&buffer[60]);
        bounding_box_zmin = *((double *)&buffer[68]);
        bounding_box_zmax = *((double *)&buffer[76]);
        bounding_box_mmin = *((double *)&buffer[84]);
        bounding_box_mmax = *((double *)&buffer[92]);
    }

    RecordHeader::RecordHeader(std::istream &stream)
    {
        uint8_t buffer[12];
        stream.read((char *)buffer, 12);

        // Little endian
        record_number = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[0]));
        content_length = satdump::swap_endian<int32_t>(*((int32_t *)&buffer[4])) * 2 - 4;

        // Big endian
        shape_type = *((int32_t *)&buffer[8]);
    }

    NullShapeRecord::NullShapeRecord(std::istream & /*stream*/, RecordHeader &header) : RecordHeader(header)
    {
        // Empty
    }

    PointRecord::PointRecord(std::istream &stream, RecordHeader &header) : RecordHeader(header)
    {
        if (header.content_length != 16)
            throw std::runtime_error("Point record should be 16-bytes, but is " + std::to_string(header.content_length) + "!");

        uint8_t buffer[16];
        stream.read((char *)buffer, 16);

        // Big endian
        point = *((point_t *)&buffer[0]);
    }

    MultiPointRecord::MultiPointRecord(std::istream &stream, RecordHeader &header) : RecordHeader(header)
    {
        uint8_t *buffer = new uint8_t[header.content_length];
        stream.read((char *)buffer, header.content_length);

        // Big endian
        box = *((box_t *)&buffer[0]);
        point_number = *((int32_t *)&buffer[32]);

        for (int i = 0; i < point_number; i++)
        {
            point_t p = *((point_t *)&buffer[36 + 8 * (i * 2)]);
            points.push_back(p);
        }

        delete[] buffer;
    }

    PolyLineRecord::PolyLineRecord(std::istream &stream, RecordHeader &header) : RecordHeader(header)
    {
        uint8_t *buffer = new uint8_t[header.content_length];
        stream.read((char *)buffer, header.content_length);

        // Big endian
        box = *((box_t *)&buffer[0]);
        part_number = *((int32_t *)&buffer[32]);
        point_number = *((int32_t *)&buffer[36]);

        std::vector<int32_t> parts; // Parse all parts
        for (int i = 0; i < part_number; i++)
        {
            int32_t part = *((int32_t *)&buffer[40 + 4 * i]);
            parts.push_back(part);
        }

        std::vector<point_t> points; // Parse all points
        for (int i = 0; i < point_number; i++)
        {
            point_t p = *((point_t *)&buffer[40 + 4 * part_number + 8 * (i * 2)]);
            points.push_back(p);
        }

        // Now get all points for each part, except the last
        for (int pn = 0; pn < part_number - 1; pn++)
        {
            std::vector<point_t> part_points;
            int part_start = parts[pn];
            int part_end = parts[pn + 1];
            part_points.insert(part_points.end(), &points[part_start], &points[part_end]);
            parts_points.push_back(part_points);
        }
        // And the last
        {
            std::vector<point_t> part_points;
            int part_start = parts[part_number - 1];
            part_points.insert(part_points.end(), &points[part_start], &points[points.size() - 1]);
            parts_points.push_back(part_points);
        }

        delete[] buffer;
    }

    PolygonRecord::PolygonRecord(std::istream &stream, RecordHeader &header) : PolyLineRecord(stream, header)
    {
    }

    Shapefile::Shapefile(std::istream &stream) : header(stream)
    {
        while (!stream.eof())
        {
            shapefile::RecordHeader recordHeader(stream);

            if (stream.tellg() < 0) // Safety
                continue;

            if (recordHeader.shape_type == shapefile::Point)
            {
                shapefile::PointRecord pointRecord(stream, recordHeader);
                point_records.push_back(pointRecord);
            }
            else if (recordHeader.shape_type == shapefile::MultiPoint)
            {
                shapefile::MultiPointRecord multiPointRecord(stream, recordHeader);
                multipoint_records.push_back(multiPointRecord);
            }
            else if (recordHeader.shape_type == shapefile::PolyLine)
            {
                shapefile::PolyLineRecord polyLineRecord(stream, recordHeader);
                polyline_records.push_back(polyLineRecord);
            }
            else if (recordHeader.shape_type == shapefile::Polygon)
            {
                shapefile::PolygonRecord polygonRecord(stream, recordHeader);
                polygon_records.push_back(polygonRecord);
            }
            else
            {
                uint8_t *buffer = new uint8_t[recordHeader.content_length];
                stream.read((char *)buffer, recordHeader.content_length);
                delete[] buffer;
            }
        }
    }
}