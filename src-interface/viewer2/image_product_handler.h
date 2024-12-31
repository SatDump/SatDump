#pragma once

#include "product_handler.h"

#include "logger.h"
#include "products2/image_product.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"
// #include "libs/ctpl/ctpl_stl.h"
// #include "common/widgets/markdown_helper.h"
// #include "common/widgets/timed_message.h"
// TODOREWORK?
#include <thread>

namespace satdump
{
    namespace viewer
    {
        class ImageProductHandler : public ProductHandler
        {
        public:
            ~ImageProductHandler();

            ImageViewWidget image_view;

            std::string equation;
            float progress = 0;

            std::shared_ptr<std::thread> wip_thread;

            // Products
            products::ImageProduct *product;

            // The Rest
            void init();
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return product->instrument_name; }

            static std::string getID() { return "image_product_handler"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<ImageProductHandler>(); }
        };
    }
}