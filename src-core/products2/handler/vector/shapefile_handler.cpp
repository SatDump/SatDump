#include "shapefile_handler.h"
#include "nlohmann/json_utils.h"

#include "logger.h"
#include "dbf_file/dbf_file.h"

#include "imgui/implot/implot.h"
#include "core/style.h"

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
            std::string db = std::filesystem::path(shapefile).parent_path().string() + "/" + std::filesystem::path(shapefile).stem().string() + ".dbf";
            shapefile_name = std::filesystem::path(shapefile).stem().string() + ".shp";
            logger->critical(db);
            if (std::filesystem::exists(db))
            {
                dbf_file = dbf_file::readDbfFile(db);
                logger->critical("JSON : \n%s\n", dbf_file.dump(4).c_str());
            }
        }

        ShapefileHandler::~ShapefileHandler()
        {
        }

        void ShapefileHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Settings"))
            {
                ImGui::ColorEdit3("##shapefilecolor##Color", (float *)&color_to_draw, ImGuiColorEditFlags_NoInputs /*| ImGuiColorEditFlags_NoLabel*/);
            }
        }

        void ShapefileHandler::drawMenuBar()
        {
        }

        void ShapefileHandler::drawContents(ImVec2 win_size)
        {
            if (ImPlot::BeginPlot("##shapefileplot", ImVec2(win_size.x, win_size.y - 16 * ui_scale), ImPlotFlags_Equal))
            {
                auto polylineDraw = [this](std::vector<std::vector<shapefile::point_t>> parts, int num)
                {
                    for (std::vector<shapefile::point_t> coordinates : parts)
                    {
                        std::vector<double> line_x, line_y;
                        for (int i = 0; i < (int)coordinates.size(); i++)
                        {
                            line_x.push_back(coordinates[i].x);
                            line_y.push_back(coordinates[i].y);
                        }

                        std::string shapeID = "Polygon " + std::to_string(num);
                        if (dbf_file[num]["NAME_1"].is_string())
                            shapeID = dbf_file[num]["NAME_1"];
                        if (dbf_file[num]["name"].is_string())
                            shapeID = dbf_file[num]["name"];

                        ImPlot::PlotLine(shapeID.c_str(), line_x.data(), line_y.data(), line_x.size());
                    }
                };

                auto pointDraw = [this](shapefile::point_t coordinates, int num)
                {
                    std::string shapeID = "Point " + std::to_string(num);
                    if (dbf_file[num]["NAME_1"].is_string())
                        shapeID = dbf_file[num]["NAME_1"];
                    if (dbf_file[num]["name"].is_string())
                        shapeID = dbf_file[num]["name"];

                    ImPlot::PlotScatter(shapeID.c_str(), &coordinates.x, &coordinates.y, 1);
                };

                for (shapefile::PolyLineRecord &polylineRecord : file->polyline_records)
                    polylineDraw(polylineRecord.parts_points, polylineRecord.record_number - 1);

                for (shapefile::PolygonRecord &polygonRecord : file->polygon_records)
                    polylineDraw(polygonRecord.parts_points, polygonRecord.record_number - 1);

                for (shapefile::PointRecord &pointRecord : file->point_records)
                    pointDraw(pointRecord.point, pointRecord.record_number - 1);

                for (shapefile::MultiPointRecord &multipointRecord : file->multipoint_records)
                    for (shapefile::point_t p : multipointRecord.points)
                        pointDraw(p, multipointRecord.record_number - 1);

                ImPlot::EndPlot();
            }
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
            std::vector<double> color = {color_to_draw.x, color_to_draw.y, color_to_draw.z, color_to_draw.w};

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
