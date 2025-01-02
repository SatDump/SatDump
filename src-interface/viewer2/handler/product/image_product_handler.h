#pragma once

#include "product_handler.h"
#include "../processing_handler.h"

#include "logger.h"
#include "products2/image_product.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"
// #include "libs/ctpl/ctpl_stl.h"
// #include "common/widgets/markdown_helper.h"
// #include "common/widgets/timed_message.h"
// TODOREWORK?
#include <thread>

#include "../image/image_handler.h"

namespace satdump
{
    namespace viewer
    {
        class ImageProductHandler : public ProductHandler, public ProcessingHandler
        {
        public:
            ImageProductHandler(std::shared_ptr<products::Product> p);
            ~ImageProductHandler();

            ImageHandler img_handler;

            std::string equation;
            float progress = 0;

            // Products
            products::ImageProduct *product;

            // Auto-update in UI
            bool needs_to_update = true;

            // Channel selection
            std::string channel_selection_box_str;
            int channel_selection_curr_id = 0;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            std::string getName() { return product->instrument_name; }

            static std::string getID() { return "image_product_handler"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<ImageProductHandler>(nullptr); }
        };
    }
}