#pragma once

#include "../image/image_handler.h"
#include "product_handler.h"
#include "products/image/calibration_converter.h"
#include "products/image/calibration_units.h"
#include "products/image/image_calibrator.h"
#include "products/image_product.h"
#include <vector>

namespace satdump
{
    namespace handlers
    {
        class ImageProductHandler : public ProductHandler
        {
        public:
            ImageProductHandler(std::shared_ptr<products::Product>, bool dataset_mode = false);
            ~ImageProductHandler();

            std::shared_ptr<ImageHandler> img_handler;

            // Advanced mode
            bool enabled_advanced_menus = false;

            // Products
            products::ImageProduct *product;
            std::shared_ptr<products::ImageCalibrator> img_calibrator;

            // Auto-update in UI
            bool needs_to_update = true;

            // Channel selection
            std::string channel_selection_box_str;
            int channel_selection_curr_id = 0;

            // Calibration : Single channels / range
            bool images_can_be_calibrated = false;
            bool channel_calibrated = false;

            struct CalibInfo
            {
                calibration::UnitInfo info;
                double min;
                double max;
            };

            std::map<int, std::map<std::string, CalibInfo>> channels_calibrated_ranges;
            std::string channels_calibrated_curr_unit;

            // Calibration : Units / Ranges for mouse overlay
            std::vector<std::pair<calibration::UnitInfo, std::shared_ptr<calibration::UnitConverter>>> channels_calibration_units_and_converters;

            void initOverlayConverters();

            // Expression
            std::string expression;
            float progress = 0;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();
            void resetConfig() { img_handler->resetConfig(); }

            void saveResult(std::string directory);

            void addSubHandler(std::shared_ptr<Handler> handler, bool ontop = false)
            {
                // Handler::addSubHandler(handler, ontop);
                img_handler->addSubHandler(handler, ontop);
            }

            void delSubHandler(std::shared_ptr<Handler> handler, bool now = false)
            { // Handler::delSubHandler(handler, now);
                img_handler->delSubHandler(handler, true);
            }

            void drawTreeMenu(std::shared_ptr<Handler> &h)
            {
                // Handler::drawTreeMenu(h);
                img_handler->drawTreeMenu(h);
            }

            std::string getID() { return "image_product_handler"; }
        };
    } // namespace handlers
} // namespace satdump