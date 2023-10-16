#pragma once

#include "viewer.h"

#include "logger.h"
#include "products/scatterometer_products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"

namespace satdump
{
    class ScatterometerViewerHandler : public ViewerHandler
    {
    public:
        // Products
        ScatterometerProducts *products;

        enum SCAT_TYPE
        {
            SCAT_ASCAT,
            SCAT_FANBEAM,
        };
        SCAT_TYPE current_scat_type;

        // Visualization
        int selected_visualization_id = 0;

        // Grayscale viz
        int select_channel_image_id = 0;
        std::string select_channel_image_str;
        ImageViewWidget image_view;
        int scat_grayscale_min = 0, scat_grayscale_max = 1e7;

        nlohmann::json current_image_proj;
        image::Image<uint16_t> current_img;

        // ASCAT-Only
        int ascat_select_channel_id = 0;
        std::string ascat_select_channel_image_str;

        // Projections
        bool use_draw_proj_algo = false;
        bool projection_ready = false, should_project = false;
        image::Image<uint16_t> projected_img;

        bool map_overlay = false;
        bool shores_overlay = false;
        bool cities_overlay = false;
        bool latlon_overlay = false;
        int cities_size = 50;

        void init();

        std::mutex async_image_mutex;
        void asyncUpdate();
        void update();

        void drawMenu();
        void drawContents(ImVec2 win_size);
        float drawTreeMenu();

        bool canBeProjected();
        bool hasProjection();
        bool shouldProject();
        void updateProjection(int width, int height, nlohmann::json settings, float *progess);
        image::Image<uint16_t> &getProjection();
        unsigned int getPreviewImageTexture() { return image_view.getTextID(); }
        void setShouldProject(bool proj) { should_project = proj; }

        static std::string getID() { return "scatterometer_handler"; }
        static std::shared_ptr<ViewerHandler> getInstance() { return std::make_shared<ScatterometerViewerHandler>(); }
    };
}