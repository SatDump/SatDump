#pragma once

#include "../handler.h"

#include "common/map/shapefile.h"
#include "common/image/image.h"

namespace satdump
{
    namespace viewer
    {
        class ShapefileHandler : public Handler
        {
        public:
            ShapefileHandler();
            ShapefileHandler(std::string shapefile);
            ~ShapefileHandler();

            std::unique_ptr<shapefile::Shapefile> file;

            void draw_to_image(image::Image &img, std::function<std::pair<double, double>(double, double, double, double)> projectionFunc);

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            std::string getName() { return "ShapefileTODOREWORK"; }

            std::string getID() { return "shapefile_handler"; }
        };
    }
}