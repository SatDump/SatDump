#pragma once

#include "viewer.h"

#include "logger.h"
#include "products/radiation_products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"

namespace satdump
{
    class RadiationViewerHandler : public ViewerHandler
    {
    public:
        // Products
        RadiationProducts *products;

        // Visualization
        int selected_visualization_id = 0;

        // Map viz
        image::Image<uint16_t> map_img;
        int select_channel_image_id = 0;
        std::string select_channel_image_str;
        ImageViewWidget image_view;
        int map_min = 0, map_max = 255;

        // Graph viz
        std::vector<std::vector<float>> graph_values;

        void init();
        void update();

        void drawMenu();
        void drawContents(ImVec2 win_size);
        float drawTreeMenu();

        static std::string getID() { return "radiation_handler"; }
        static std::shared_ptr<ViewerHandler> getInstance() { return std::make_shared<RadiationViewerHandler>(); }

        // Projections
        bool use_draw_proj_algo = false;
        bool projection_ready = false, should_project = false;
        image::Image<uint16_t> projected_img;

        bool canBeProjected();
        bool hasProjection();
        bool shouldProject();
        void updateProjection(int width, int height, nlohmann::json settings, float *progess);
        image::Image<uint16_t> &getProjection();
        unsigned int getPreviewImageTexture() { return image_view.getTextID(); }
        void setShouldProject(bool proj) { should_project = proj; }
    };
}