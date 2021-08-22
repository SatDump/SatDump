#include "map_drawer.h"
#include "nlohmann/json.hpp"
#include <fstream>

namespace map
{
    template <typename T>
    void drawProjectedMap(cimg_library::CImg<T> &map_image, std::vector<std::string> shapeFiles, T color[3], std::function<std::pair<int, int>(float, float, int, int)> projectionFunc)
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

                            if (start.first == -1 || end.first == -1)
                                continue;

                            map_image.draw_line(start.first, start.second, end.first, end.second, color);
                        }

                        {
                            std::pair<float, float> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                           map_image.height(), map_image.width());
                            std::pair<float, float> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                         map_image.height(), map_image.width());

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

                        if (start.first == -1 || end.first == -1)
                            continue;

                        map_image.draw_line(start.first, start.second, end.first, end.second, color);
                    }

                    {
                        std::pair<float, float> start = projectionFunc(coordinates[0].second, coordinates[0].first,
                                                                       map_image.height(), map_image.width());
                        std::pair<float, float> end = projectionFunc(coordinates[coordinates.size() - 1].second, coordinates[coordinates.size() - 1].first,
                                                                     map_image.height(), map_image.width());

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

                    map_image.draw_point(cc.first, cc.second, color);
                }
            }
        }
    }

    template void drawProjectedMap(cimg_library::CImg<unsigned char> &, std::vector<std::string>, unsigned char[3], std::function<std::pair<int, int>(float, float, int, int)>);
    template void drawProjectedMap(cimg_library::CImg<unsigned short> &, std::vector<std::string>, unsigned short[3], std::function<std::pair<int, int>(float, float, int, int)>);
}