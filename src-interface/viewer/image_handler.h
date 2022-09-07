#pragma once

#include "viewer.h"

#include "logger.h"
#include "products/image_products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"
#include "common/image/composite.h"
#include "main_ui.h"
#include "core/style.h"
#include "imgui/pfd/portable-file-dialogs.h"

namespace satdump
{
    class ImageViewerHandler : public ViewerHandler
    {
    public:
        // Products
        ImageProducts *products;

        // Image handling
        int active_channel_id = 0, select_image_id = 1;
        std::string select_image_str;
        image::Image<uint16_t> rgb_image, current_image;
        std::vector<image::Image<uint16_t>> images_obj;
        ImageViewWidget image_view;

        // Other controls
        bool median_blur = false;
        bool rotate_image = false;
        bool equalize_image = false;
        bool invert_image = false;
        bool correct_image = false;
        bool normalize_image = false;
        bool white_balance_image = false;

        bool map_overlay = false;
        bool cities_overlay = false;

        float cities_scale = 0.5;

        // RGB Handling
        ImageCompositeCfg rgb_compo_cfg;
        std::vector<std::string> channel_numbers;
        float rgb_progress = 0;
        bool rgb_processing = false;
        std::vector<std::pair<std::string, ImageCompositeCfg>> rgb_presets;
        std::string rgb_presets_str;
        int select_rgb_presets = -1;

        std::vector<double> current_timestamps;

        /*
        // Warp/Project
        int warp_project_width = 2048;
        int warp_project_height = 1024;
        */

        void init();
        void updateImage();

        std::mutex async_image_mutex;
        void asyncUpdate();
        void updateRGB();

        void drawMenu();
        void drawContents(ImVec2 win_size);
        float drawTreeMenu();

        bool canBeProjected();
        bool hasProjection();
        bool updateProjection(int width, int height, nlohmann::json settings);
        image::Image<uint16_t> &getProjection();

        static std::string getID() { return "image_handler"; }
        static std::shared_ptr<ViewerHandler> getInstance() { return std::make_shared<ImageViewerHandler>(); }
    };
}