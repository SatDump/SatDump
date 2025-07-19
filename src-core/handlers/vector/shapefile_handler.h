#pragma once

#include "../handler.h"

#include "common/map/shapefile.h"
#include "image/image.h"

namespace satdump
{
    namespace handlers
    {
        class ShapefileHandler : public Handler
        {
        public:
            ShapefileHandler();
            ShapefileHandler(std::string shapefile);
            ~ShapefileHandler();

            std::string shapefile_name;
            std::unique_ptr<shapefile::Shapefile> file;
            nlohmann::json dbf_file;
            bool has_dbf = false;

            ImVec4 color_to_draw = {0, 1, 0, 1};
            int font_size = 20;
            int scalerank_filter = 3;

            void draw_to_image(image::Image &img, std::function<std::pair<double, double>(double, double, double, double)> projectionFunc);

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            std::string getName() { return shapefile_name; }

            std::string getID() { return "shapefile_handler"; }
        };
    } // namespace handlers
} // namespace satdump