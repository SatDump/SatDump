#pragma once

/**
 * @file image_handler.h
 */

#include "../handler.h"
#include "../processing_handler.h"

#include "common/widgets/image_view.h"
#include "handlers/image/image_filter.h"
#include "image/meta.h"

#include "nlohmann/json.hpp"
#include "projection/projection.h"
#include <memory>

namespace satdump
{
    namespace handlers
    {
        // TODOREWORK
        struct ActiveFilter
        {
            nlohmann::json cfg;
            bool enabled = true;
            float progress = 0;
        };

        inline void to_json(nlohmann::json &j, const ActiveFilter &v) { j = v.cfg; }
        inline void from_json(const nlohmann::json &j, ActiveFilter &v) { v.cfg = j; }

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
            int rotate_image = 0;
            bool geocorrect_image = false;

            // Experimental
            std::map<std::string, ImageFilter> image_filters;
            std::vector<std::pair<std::string, ActiveFilter>> active_filters;
            std::shared_ptr<ImageFilterConfigurator> image_filter_configurator;
            int image_filter_configurator_set_in = -1;

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
            // Allow disabling overlay menu, eg, for projections where they are separate
            bool enableOverlayMenu = true;

            // If the image subhandlers are not exposed, we need to send the crops somewhere else
            bool sendCropToRoot = false;

            // For projections
            bool removeProjectionInfoFromCrop = false;

        public:
            /**
             * @brief Return a sane image name for saving
             * @return cleaned-up name. TODOREWORK generalize?
             */
            std::string getSaneName();

            // TODOREWORK document?
            bool saveResult(std::string directory);

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
                rotate_image = 0;
                geocorrect_image = false;

                image.clear();
                curr_image.clear();
                active_filters.clear();
            }

            std::string getName() { return image_name; }

            std::string getID() { return "image_handler"; }
        };
    } // namespace handlers
} // namespace satdump