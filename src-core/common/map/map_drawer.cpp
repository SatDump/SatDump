#define _CRT_NO_VA_START_VALIDATION
#include "map_drawer.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "shapefile.h"

namespace map
{
    template <typename T>
    void drawProjectedMapGeoJson(std::vector<std::string> shapeFiles, image::Image<T> &map_image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, int maxLength)
    {
        for (std::string currentShapeFile : shapeFiles)
        {
            nlohmann::json shapeFile;
            {
                std::ifstream istream(currentShapeFile);
                istream >> shapeFile;
                istream.close();
            }

            for (const nlohmann::json &mapStruct : shapeFile.at("features"))
            {
                if (mapStruct["type"] != "Feature")
                    continue;

                std::string geometryType = mapStruct["geometry"]["type"];

                if (geometryType == "Polygon")
                {
                    std::vector<std::vector<std::pair<float, float>>> all_coordinates = mapStruct["geometry"]["coordinates"].get<std::vector<std::vector<std::pair<float, float>>>>();

                    for (std::vector<std::pair<float, float>> coordinates : all_coordinates)
                    {
                        for (int i = 0; i < (int)coordinates.size() - 1; i++)
                        {
                            std::pair<float, float> start = projectionFunc(coordinates[i].second, coordinates[i].first,
                                                                           map_image.height(), map_image.width());
                            std::pair<float, float> end = projectionFunc(coordinates[i + 1].second, coordinates[i + 1].first,
                                                                         map_image.height(), map_image.width());

                            if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                continue;

                            if (start.first == -1 || end.first == -1)
                                continue;

                            map_image.draw_line(start.first, start.second, end.first, end.second, color);
                        }

                        {
                            std::pair<float, float> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                           map_image.height(), map_image.width());
                            std::pair<float, float> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                         map_image.height(), map_image.width());

                            if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                continue;

                            if (start.first == -1 || end.first == -1)
                                continue;

                            map_image.draw_line(start.first, start.second, end.first, end.second, color);
                        }
                    }
                }
                else if (geometryType == "MultiPolygon")
                {
                    std::vector<std::vector<std::vector<std::pair<float, float>>>> all_all_coordinates = mapStruct["geometry"]["coordinates"].get<std::vector<std::vector<std::vector<std::pair<float, float>>>>>();

                    for (std::vector<std::vector<std::pair<float, float>>> all_coordinates : all_all_coordinates)
                    {
                        for (std::vector<std::pair<float, float>> coordinates : all_coordinates)
                        {
                            for (int i = 0; i < (int)coordinates.size() - 1; i++)
                            {
                                std::pair<float, float> start = projectionFunc(coordinates[i].second, coordinates[i].first,
                                                                               map_image.height(), map_image.width());
                                std::pair<float, float> end = projectionFunc(coordinates[i + 1].second, coordinates[i + 1].first,
                                                                             map_image.height(), map_image.width());

                                if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                    continue;

                                if (start.first == -1 || end.first == -1)
                                    continue;

                                map_image.draw_line(start.first, start.second, end.first, end.second, color);
                            }

                            {
                                std::pair<float, float> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                               map_image.height(), map_image.width());

                                std::pair<float, float> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                             map_image.height(),
                                                                             map_image.width());
                                if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                                    continue;

                                if (start.first == -1 || end.first == -1)
                                    continue;

                                map_image.draw_line(start.first, start.second, end.first, end.second, color);
                            }
                        }
                    }
                }
                else if (geometryType == "LineString")
                {
                    std::vector<std::pair<float, float>> coordinates = mapStruct["geometry"]["coordinates"].get<std::vector<std::pair<float, float>>>();

                    for (int i = 0; i < (int)coordinates.size() - 1; i++)
                    {
                        std::pair<float, float> start = projectionFunc(coordinates[i].second, coordinates[i].first,
                                                                       map_image.height(), map_image.width());
                        std::pair<float, float> end = projectionFunc(coordinates[i + 1].second, coordinates[i + 1].first,
                                                                     map_image.height(), map_image.width());

                        if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                            continue;

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }

                    {
                        std::pair<float, float> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                       map_image.height(), map_image.width());
                        std::pair<float, float> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                     map_image.height(), map_image.width());

                        if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                            continue;

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }
                }
                else if (geometryType == "Point")
                {
                    std::pair<float, float> coordinates = mapStruct["geometry"]["coordinates"].get<std::pair<float, float>>();
                    std::pair<float, float> cc = projectionFunc(coordinates.second, coordinates.first,
                                                                map_image.height(), map_image.width());

                    if (cc.first == -1 || cc.first == -1)
                        continue;

                    map_image.draw_pixel(cc.first, cc.second, color);
                }
            }
        }
    }

    template void drawProjectedMapGeoJson(std::vector<std::string>, image::Image<uint8_t> &, uint8_t[3], std::function<std::pair<int, int>(float, float, int, int)>, int);
    template void drawProjectedMapGeoJson(std::vector<std::string>, image::Image<uint16_t> &, uint16_t[3], std::function<std::pair<int, int>(float, float, int, int)>, int);

    template <typename T>
    void drawProjectedCapitalsGeoJson(std::vector<std::string> shapeFiles, image::Image<T> &map_image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, float ratio)
    {
        std::vector<image::Image<uint8_t>> font = image::make_font(50 * ratio);

        for (std::string currentShapeFile : shapeFiles)
        {
            nlohmann::json shapeFile;
            {
                std::ifstream istream(currentShapeFile);
                istream >> shapeFile;
                istream.close();
            }

            for (const nlohmann::json &mapStruct : shapeFile.at("features"))
            {
                if (mapStruct["type"] != "Feature")
                    continue;

                std::string geometryType = mapStruct["geometry"]["type"];

                if (geometryType == "Point")
                {
                    if (mapStruct["properties"]["featurecla"].get<std::string>() == "Admin-0 capital")
                    {
                        std::pair<float, float> coordinates = mapStruct["geometry"]["coordinates"].get<std::pair<float, float>>();
                        std::pair<float, float> cc = projectionFunc(coordinates.second, coordinates.first,
                                                                    map_image.height(), map_image.width());

                        if (cc.first == -1 || cc.first == -1)
                            continue;

                        map_image.draw_line(cc.first - 20 * ratio, cc.second - 20 * ratio, cc.first + 20 * ratio, cc.second + 20 * ratio, color);
                        map_image.draw_line(cc.first + 20 * ratio, cc.second - 20 * ratio, cc.first - 20 * ratio, cc.second + 20 * ratio, color);
                        map_image.draw_circle(cc.first, cc.second, 10 * ratio, color, true);

                        std::string name = mapStruct["properties"]["nameascii"];
                        map_image.draw_text(cc.first, cc.second + 20 * ratio, color, font, name);
                    }
                }
            }
        }
    }

    template void drawProjectedCapitalsGeoJson(std::vector<std::string>, image::Image<uint8_t> &, uint8_t[3], std::function<std::pair<int, int>(float, float, int, int)>, float);
    template void drawProjectedCapitalsGeoJson(std::vector<std::string>, image::Image<uint16_t> &, uint16_t[3], std::function<std::pair<int, int>(float, float, int, int)>, float);

    template <typename T>
    void drawProjectedMapShapefile(std::vector<std::string> shapeFiles, image::Image<T> &map_image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, int maxLength)
    {
        for (std::string currentShapeFile : shapeFiles)
        {
            std::ifstream inputFile(currentShapeFile, std::ios::binary);
            shapefile::Shapefile shape_file(inputFile);

            std::function<void(std::vector<std::vector<shapefile::point_t>>)> polylineDraw = [color, maxLength, &map_image, &projectionFunc](std::vector<std::vector<shapefile::point_t>> parts)
            {
                for (std::vector<shapefile::point_t> coordinates : parts)
                {
                    for (int i = 0; i < (int)coordinates.size() - 1; i++)
                    {
                        std::pair<float, float> start = projectionFunc(coordinates[i].y, coordinates[i].x,
                                                                       map_image.height(), map_image.width());
                        std::pair<float, float> end = projectionFunc(coordinates[i + 1].y, coordinates[i + 1].x,
                                                                     map_image.height(), map_image.width());

                        if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                            continue;

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }

                    {
                        std::pair<float, float> start = projectionFunc(coordinates[0].y, coordinates[0].x,
                                                                       map_image.height(), map_image.width());
                        std::pair<float, float> end = projectionFunc(coordinates[coordinates.size() - 1].y, coordinates[coordinates.size() - 1].x,
                                                                     map_image.height(), map_image.width());

                        if (sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2)) >= maxLength)
                            continue;

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }
                }
            };

            std::function<void(shapefile::point_t)> pointDraw = [color, &map_image, &projectionFunc](shapefile::point_t coordinates)
            {
                std::pair<float, float> cc = projectionFunc(coordinates.y, coordinates.x,
                                                            map_image.height(), map_image.width());

                if (cc.first == -1 || cc.first == -1)
                    return;

                map_image.draw_pixel(cc.first, cc.second, color);
            };

            for (shapefile::PolyLineRecord polylineRecord : shape_file.polyline_records)
                polylineDraw(polylineRecord.parts_points);

            for (shapefile::PolygonRecord polygonRecord : shape_file.polygon_records)
                polylineDraw(polygonRecord.parts_points);

            for (shapefile::PointRecord pointRecord : shape_file.point_records)
                pointDraw(pointRecord.point);

            for (shapefile::MultiPointRecord multipointRecord : shape_file.multipoint_records)
                for (shapefile::point_t p : multipointRecord.points)
                    pointDraw(p);
        }
    }

    template void drawProjectedMapShapefile(std::vector<std::string>, image::Image<uint8_t> &, uint8_t[3], std::function<std::pair<int, int>(float, float, int, int)>, int);
    template void drawProjectedMapShapefile(std::vector<std::string>, image::Image<uint16_t> &, uint16_t[3], std::function<std::pair<int, int>(float, float, int, int)>, int);

    template <typename T>
    void drawProjectedLabels(std::vector<CustomLabel> labels, image::Image<T> &image, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc, float ratio)
    {
        std::vector<image::Image<uint8_t>> font = image::make_font(50 * ratio);

        for (CustomLabel &currentLabel : labels)
        {
            std::pair<float, float> cc = projectionFunc(currentLabel.lat, currentLabel.lon,
                                                        image.height(), image.width());

            if (cc.first == -1 || cc.first == -1)
                continue;

            image.draw_line(cc.first - 20 * ratio, cc.second - 20 * ratio, cc.first + 20 * ratio, cc.second + 20 * ratio, color);
            image.draw_line(cc.first + 20 * ratio, cc.second - 20 * ratio, cc.first - 20 * ratio, cc.second + 20 * ratio, color);
            image.draw_circle(cc.first, cc.second, 10 * ratio, color, true);

            image.draw_text(cc.first, cc.second + 20 * ratio, color, font, currentLabel.label);
        }
    }

    template void drawProjectedLabels(std::vector<CustomLabel>, image::Image<uint8_t> &, uint8_t[3], std::function<std::pair<int, int>(float, float, int, int)>, float);
    template void drawProjectedLabels(std::vector<CustomLabel>, image::Image<uint16_t> &, uint16_t[3], std::function<std::pair<int, int>(float, float, int, int)>, float);
}