#include "shapefile_handler.h"
#include "nlohmann/json_utils.h"

namespace satdump
{
    namespace viewer
    {
        ShapefileHandler::ShapefileHandler()
        {
        }

        ShapefileHandler::ShapefileHandler(std::string shapefile)
        {
            std::ifstream f(shapefile, std::ios::binary);
            file = std::make_unique<shapefile::Shapefile>(f);
        }

        ShapefileHandler::~ShapefileHandler()
        {
        }

        void ShapefileHandler::drawMenu()
        {
        }

        void ShapefileHandler::drawMenuBar()
        {
        }

        void ShapefileHandler::drawContents(ImVec2 win_size)
        {
        }

        void ShapefileHandler::setConfig(nlohmann::json p)
        {
        }

        nlohmann::json ShapefileHandler::getConfig()
        {
            nlohmann::json p;
            return p;
        }

        void ShapefileHandler::draw_to_image(image::Image &img, std::function<std::pair<double, double>(double, double, double, double)> projectionFunc)
        {
            // TODOREWORK
            std::vector<double> color = {1, 0, 0, 1};

            {
                std::function<void(std::vector<std::vector<shapefile::point_t>>)> polylineDraw = [color, &img, &projectionFunc](std::vector<std::vector<shapefile::point_t>> parts)
                {
                    int width = img.width();
                    int height = img.height();

                    for (std::vector<shapefile::point_t> coordinates : parts)
                    {
                        std::pair<double, double> start = projectionFunc(coordinates[0].y, coordinates[0].x, height, width);
                        for (int i = 1; i < (int)coordinates.size() - 1; i++)
                        {
                            std::pair<double, double> end = projectionFunc(coordinates[i].y, coordinates[i].x, height, width);
                            if (start.first != -1 && start.second != -1 && end.first != -1 && end.second != -1)
                                img.draw_line(start.first, start.second, end.first, end.second, color);

                            start = end;
                        }
                    }
                };

                std::function<void(shapefile::point_t)> pointDraw = [color, &img, &projectionFunc](shapefile::point_t coordinates)
                {
                    std::pair<double, double> cc = projectionFunc(coordinates.y, coordinates.x,
                                                                  img.height(), img.width());

                    if (cc.first == -1 || cc.second == -1)
                        return;

                    img.draw_pixel(cc.first, cc.second, color);
                };

                for (shapefile::PolyLineRecord &polylineRecord : file->polyline_records)
                    polylineDraw(polylineRecord.parts_points);

                for (shapefile::PolygonRecord &polygonRecord : file->polygon_records)
                    polylineDraw(polygonRecord.parts_points);

                for (shapefile::PointRecord &pointRecord : file->point_records)
                    pointDraw(pointRecord.point);

                for (shapefile::MultiPointRecord &multipointRecord : file->multipoint_records)
                    for (shapefile::point_t p : multipointRecord.points)
                        pointDraw(p);
            }
        }
    }
}
