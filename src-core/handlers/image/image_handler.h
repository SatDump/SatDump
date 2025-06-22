#pragma once

/**
 * @file image_handler.h
 */

#include "../handler.h"
#include "../processing_handler.h"

#include "common/widgets/image_view.h"
#include "image/hue_saturation_json.h"
#include "image/meta.h"

#include "nlohmann/json.hpp"
#include "projection/projection.h"

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief The main standard ImageHandler.
         *
         * This is more or less just meant to display images. It
         * may either be used directly to present an image to the user
         * in the explorer, while also showing edition options (TODOREWORK,
         * have a mode where (part of?) editing is disabled?), or
         * integrated in another handler as you wish.
         *
         * It may also be used to perform various image processing tasks. TODOREWORK document all functions!
         */
        class ImageHandler : public Handler, public ProcessingHandler
        {
        public:
            /**
             * @brief Constructor
             */
            ImageHandler();

            /**
             * @brief Constructor, init with an image
             * @param img image to be initialized with
             */
            ImageHandler(image::Image img);

            /**
             * @brief Constructor, init with image and name
             * @param img image to be initialized with
             * @param name of the handler to display
             */
            ImageHandler(image::Image img, std::string name);

            ~ImageHandler();

        private:
            std::string image_name = "Unknown Image";

            bool imgview_needs_update = false;
            bool has_second_image = false;
            image::Image image, curr_image;
            ImageViewWidget image_view;

        public:
            /**
             * @brief set current image (raw). Does
             * NOT reset settings
             * @param img image to set
             */
            void setImage(image::Image &img);

            /**
             * @brief Get current image. Either
             * processed or raw.
             * @param current If true, returns the
             * *processed* image. Raw if false.
             */
            image::Image &getImage(bool current = true)
            {
                if (current)
                    return has_second_image ? curr_image : image;
                else
                    return image;
            }

            /**
             * @brief Set the image name
             * @param name new name to set
             */
            void setName(std::string name) { image_name = name; }

        private:
            // All params
            bool huesaturation_img = false;
            image::HueSaturation huesaturation_cfg_img;
            bool equalize_img = false;
            bool equalize_perchannel_img = false;
            bool white_balance_img = false;
            bool normalize_img = false;
            bool invert_img = false;
            bool median_blur_img = false;
            bool despeckle_img = false;
            bool rotate180_image = false;
            bool geocorrect_image = false;
            bool brightness_contrast_image = false;
            float brightness_contrast_brightness_image = 0.0;
            float brightness_contrast_contrast_image = 0.0;
            bool remove_background_img = false;

            // Proj/Calib TODOREWORK
            bool image_calib_valid = false;
            image::ImgCalibHandler image_calib;
            bool image_proj_valid = false;
            projection::Projection image_proj;
            std::vector<float> correct_fwd_lut; // TODOREWORK handle this better?
            std::vector<float> correct_rev_lut;

            // TODOREWORK File save
            bool file_save_thread_running = false;
            std::thread file_save_thread;

            // Proc function
            void do_process();

        public:
            // Mouse callback to be added by other handlers if needed
            std::function<void(int x, int y)> additionalMouseCallback = [](int, int) {};
            // TODOREWORK
            bool wasMenuTriggered = false;

        public:
            /**
             * @brief Return a sane image name for saving
             * @return cleaned-up name. TODOREWORK generalize?
             */
            std::string getSaneName();

            // TODOREWORK document?
            void saveResult(std::string directory);

            /**
             * @brief Draws the same image Menu Item
             */
            void drawSaveMenu();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            // TODOREWORK DOCUMENT (must be copy-pasted from "All Params")
            void resetConfig()
            {
                huesaturation_img = false;
                equalize_img = false;
                equalize_perchannel_img = false;
                white_balance_img = false;
                normalize_img = false;
                invert_img = false;
                median_blur_img = false;
                despeckle_img = false;
                rotate180_image = false;
                geocorrect_image = false;
                brightness_contrast_image = false;
                brightness_contrast_brightness_image = 0.0;
                brightness_contrast_contrast_image = 0.0;
                remove_background_img = false;
            }

            std::string getName() { return image_name; }

            std::string getID() { return "image_handler"; }
        };
    } // namespace handlers
} // namespace satdump