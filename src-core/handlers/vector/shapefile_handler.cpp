#include "shapefile_handler.h"

#include "core/config.h"
#include "core/resources.h"
#include "dbf_file/dbf_file.h"
#include "image/text.h"
#include "imgui/imgui.h"
#include "logger.h"

#include "core/style.h"
#include "imgui/implot/implot.h"

namespace satdump
{
    namespace handlers
    {
        ShapefileHandler::ShapefileHandler() { handler_tree_icon = u8"\uf84c"; }

        ShapefileHandler::ShapefileHandler(std::string shapefile)
        {
            handler_tree_icon = u8"\uf84c";

            std::ifstream f(shapefile, std::ios::binary);
            file = std::make_unique<shapefile::Shapefile>(f);
            std::string db = std::filesystem::path(shapefile).parent_path().string() + "/" + std::filesystem::path(shapefile).stem().string() + ".dbf";
            shapefile_name = std::filesystem::path(shapefile).stem().string() + ".shp";
            logger->critical(db);
            if (std::filesystem::exists(db))
            {
                dbf_file = dbf_file::readDbfFile(db);
                logger->critical("JSON : \n%s\n", dbf_file.dump(4).c_str());
                has_dbf = true;
            }

            if (!satdump_cfg.main_cfg["user"]["shapefile_defaults"][std::filesystem::path(shapefile_name).stem().string()].is_null())
                setConfig(satdump_cfg.main_cfg["user"]["shapefile_defaults"][std::filesystem::path(shapefile_name).stem().string()]);
        }

        ShapefileHandler::~ShapefileHandler()
        {
            satdump_cfg.main_cfg["user"]["shapefile_defaults"][std::filesystem::path(shapefile_name).stem().string()] = getConfig();
            satdump_cfg.saveUser();
        }

        void ShapefileHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit3("Draw Color##shapefilecolor", (float *)&color_to_draw, ImGuiColorEditFlags_NoInputs /*| ImGuiColorEditFlags_NoLabel*/);
                ImGui::InputInt("Font Size", &font_size);
                if (has_dbf)
                    ImGui::InputInt("Scale Rank (Cities Only)", &scalerank_filter);
            }
        }

        void ShapefileHandler::drawMenuBar() {}

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
                        if (has_dbf)
                        {
                            if (dbf_file[num]["NAME_1"].is_string())
                                shapeID = dbf_file[num]["NAME_1"];
                            if (dbf_file[num]["name"].is_string())
                                shapeID = dbf_file[num]["name"];
                        }

                        ImPlot::PlotLine(shapeID.c_str(), line_x.data(), line_y.data(), line_x.size());
                    }
                };

                auto pointDraw = [this](shapefile::point_t coordinates, int num)
                {
                    std::string shapeID = "Point " + std::to_string(num);
                    if (has_dbf)
                    {
                        if (dbf_file[num]["NAME_1"].is_string())
                            shapeID = dbf_file[num]["NAME_1"];
                        if (dbf_file[num]["name"].is_string())
                            shapeID = dbf_file[num]["name"];
                    }

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
            if (p.contains("color"))
            {
                auto &c = p["color"];
                color_to_draw.x = c[0];
                color_to_draw.y = c[1];
                color_to_draw.z = c[2];
                color_to_draw.w = c[3];
            }
            if (p.contains("font_size"))
                font_size = p["font_size"];
            if (p.contains("scalerank_filter"))
                scalerank_filter = p["scalerank_filter"];
        }

        nlohmann::json ShapefileHandler::getConfig()
        {
            nlohmann::json p;
            p["color"] = {color_to_draw.x, color_to_draw.y, color_to_draw.z, color_to_draw.w};
            p["font_size"] = font_size;
            p["scalerank_filter"] = scalerank_filter;
            return p;
        }

        void ShapefileHandler::draw_to_image(image::Image &img, std::function<std::pair<double, double>(double, double, double, double)> projectionFunc)
        {
            if (img.channels() < 3)
                img.to_rgb();

            // TODOREWORK
            std::vector<double> color = {color_to_draw.x, color_to_draw.y, color_to_draw.z, color_to_draw.w};

            image::TextDrawer text_drawer;
            text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));

            {
                std::function<void(std::vector<std::vector<shapefile::point_t>>, int num)> polylineDraw = [color, &img, &projectionFunc](std::vector<std::vector<shapefile::point_t>> parts, int num)
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

                std::function<void(shapefile::point_t, int num)> pointDraw = [this, color, &text_drawer, &img, &projectionFunc](shapefile::point_t coordinates, int num)
                {
                    std::pair<double, double> cc = projectionFunc(coordinates.y, coordinates.x, img.height(), img.width());

                    if (cc.first == -1 || cc.second == -1)
                        return;

                    if (has_dbf)
                    {
                        if (dbf_file[num]["scalerank"].is_string())
                            if (scalerank_filter < std::stoi(dbf_file[num]["scalerank"].get<std::string>()))
                                return;
                    }

#if 0
                     img.draw_pixel(cc.first, cc.second, color);
#else
                    img.draw_line(cc.first - font_size * 0.3, cc.second - font_size * 0.3, cc.first + font_size * 0.3, cc.second + font_size * 0.3, color);
                    img.draw_line(cc.first + font_size * 0.3, cc.second - font_size * 0.3, cc.first - font_size * 0.3, cc.second + font_size * 0.3, color);
                    img.draw_circle(cc.first, cc.second, 0.15 * font_size, color, true);

                    std::string shapeID;
                    if (has_dbf)
                    {
                        if (dbf_file[num]["NAME_1"].is_string())
                            shapeID = dbf_file[num]["NAME_1"];
                        if (dbf_file[num]["name"].is_string())
                            shapeID = dbf_file[num]["name"];
                    }

                    if (shapeID.size())
                        text_drawer.draw_text(img, cc.first, cc.second + font_size * 0.15, color, font_size, shapeID);
#endif
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
            }
        }
    } // namespace handlers
} // namespace satdump
