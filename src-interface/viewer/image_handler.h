#pragma once

#include "viewer.h"

#include "logger.h"
#include "products/image_products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"
#include "libs/ctpl/ctpl_stl.h"
#include "common/widgets/markdown_helper.h"
#include "common/widgets/timed_message.h"

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
        image::Image rgb_image, current_image;
        ImageViewWidget image_view;

        // Map projection stuff
        std::function<std::pair<int, int>(float, float, int, int)> proj_func;
        nlohmann::json last_proj_cfg;
        bool last_correct_image = false;
        bool last_rotate_image = false;
        size_t last_width = 0;
        size_t last_height = 0;

        // Other controls
        bool remove_background = false;
        bool median_blur = false;
        bool despeckle = false;
        bool rotate_image = false;
        bool equalize_image = false;
        bool individual_equalize_image = false;
        bool invert_image = false;
        bool correct_image = false;
        bool normalize_image = false;
        bool white_balance_image = false;
        bool manual_brightness_contrast = false;
        float manual_brightness_contrast_brightness = 0.0;
        float manual_brightness_contrast_constrast = 0.0;

        // GUI
        bool range_window = false;
        std::vector<std::pair<double, double>> disaplay_ranges;
        bool update_needed;
        bool is_updating = false;

        OverlayHandler overlay_handler;

        // Calibration
        bool is_temp = false;
        bool show_scale = false;
        image::Image scale_image; // 512x25
        ImageViewWidget scale_view;

        // LUT
        bool using_lut = false;
        image::Image lut_image;

        // Geo-Correction
        std::vector<int> correction_factors;

        // RGB Handling
        ImageCompositeCfg rgb_compo_cfg;
        std::vector<std::string> channel_numbers;
        float rgb_progress = 0;
        bool rgb_processing = false;
        std::vector<std::pair<std::string, ImageCompositeCfg>> rgb_presets;
        std::string preset_search_str;
        int select_rgb_presets = -1;

        bool show_markdown_description = false;
        widgets::MarkdownHelper markdown_composite_info;

        std::vector<double> current_timestamps;
        nlohmann::json current_proj_metadata;

        bool projection_use_old_algo = false;

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
        void addCurrentToProjections();

        widgets::TimedMessage proj_notif;

        static std::string getID() { return "image_handler"; }
        static std::shared_ptr<ViewerHandler> getInstance() { return std::make_shared<ImageViewerHandler>(); }
    };
}