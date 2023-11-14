#pragma once

#include "viewer.h"

#include "logger.h"
#include "products/image_products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"
#include "common/image/composite.h"
#include "core/style.h"
#include "libs/ctpl/ctpl_stl.h"
#include "common/widgets/markdown_helper.h"

namespace satdump
{
    class ImageViewerHandler : public ViewerHandler
    {
    public:
        ~ImageViewerHandler();

        // Products
        ImageProducts *products;

        // Image handling
        int active_channel_id = 0, select_image_id = 1;
        bool active_channel_calibrated = false;
        std::string select_image_str;
        image::Image<uint16_t> rgb_image, current_image;
        std::vector<image::Image<uint16_t>> images_obj;
        ImageViewWidget image_view;

        // Other controls
        bool median_blur = false;
        bool despeckle = false;
        bool rotate_image = false;
        bool equalize_image = false;
        bool individual_equalize_image = false;
        bool invert_image = false;
        bool correct_image = false;
        bool normalize_image = false;
        bool white_balance_image = false;

        // GUI
        bool range_window = false;
        std::vector<std::pair<double, double>> disaplay_ranges;
        bool update_needed;
        bool is_updating = false;

        OverlayHandler overlay_handler;

        // Calibration
        bool is_temp = false;
        bool show_scale = false;
        image::Image<uint16_t> scale_image; // 512x25
        ImageViewWidget scale_view;

        // LUT
        bool using_lut = false;
        image::Image<uint16_t> lut_image;

        // Geo-Correction
        std::vector<int> correction_factors;

        // RGB Handling
        ImageCompositeCfg rgb_compo_cfg;
        std::vector<std::string> channel_numbers;
        float rgb_progress = 0;
        bool rgb_processing = false;
        std::vector<std::pair<std::string, ImageCompositeCfg>> rgb_presets;
        std::string rgb_presets_str;
        int select_rgb_presets = -1;

        bool show_markdown_description = false;
        widgets::MarkdownHelper markdown_composite_info;

        std::vector<double> current_timestamps;
        nlohmann::json current_proj_metadata;

        // Projections
        bool projection_ready = false, should_project = false;
        image::Image<uint16_t> projected_img;

        bool project_old_algorithm = false;

        // Utils
        void updateScaleImage();
        void updateCorrectionFactors(bool first = false);

        // The Rest
        void init();
        void updateImage();

        ctpl::thread_pool handler_thread_pool = ctpl::thread_pool(1);
        std::mutex async_image_mutex;
        void asyncUpdate();
        void updateRGB();

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

        static std::string getID() { return "image_handler"; }
        static std::shared_ptr<ViewerHandler> getInstance() { return std::make_shared<ImageViewerHandler>(); }
    };
}