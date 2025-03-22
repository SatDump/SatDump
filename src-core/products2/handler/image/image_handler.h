#pragma once

#include "../handler.h"
#include "../processing_handler.h"

#include "logger.h"
#include "common/widgets/image_view.h"
#include "common/image/meta.h"

#include "projection/projection.h"

namespace satdump
{
    namespace viewer
    {
        class ImageHandler : public Handler, public ProcessingHandler
        {
        public:
            ImageHandler();
            ImageHandler(image::Image img);
            ~ImageHandler();

            std::string image_name = "Unknown Image";

            bool imgview_needs_update = false;
            bool has_second_image = false;
            image::Image image, curr_image;
            ImageViewWidget image_view;

            image::Image &get_current_img() { return has_second_image ? curr_image : image; }

            void updateImage(image::Image &img); // TODOREWORK

            // All params
            bool equalize_img = false;
            bool equalize_perchannel_img = false;
            bool white_balance_img = false;
            bool normalize_img = false;
            bool median_blur_img = false;
            bool despeckle_img = false;
            bool rotate180_image = false;
            bool geocorrect_image = false;

            // Proj/Calib TODOREWORK
            bool image_calib_valid = false;
            image::ImgCalibHandler image_calib;
            bool image_proj_valid = false;
            proj::Projection image_proj;
            std::vector<float> correct_fwd_lut; // TODOREWORK handle this better?
            std::vector<float> correct_rev_lut;

            // TODOREWORK File save
            bool file_save_thread_running = false;
            std::thread file_save_thread;

            // Proc function
            void do_process();

            // Mouse callback to be added by other handlers if needed
            std::function<void(int x, int y)> additionalMouseCallback = [](int, int) {};

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            std::string getName() { return image_name; }

            std::string getID() { return "image_handler"; }
        };
    }
}