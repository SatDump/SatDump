#pragma once

#include "viewer.h"

#include "logger.h"
#include "products2/image_products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"
// #include "libs/ctpl/ctpl_stl.h"
// #include "common/widgets/markdown_helper.h"
// #include "common/widgets/timed_message.h"

namespace satdump
{
    class ImageViewerHandler2 : public ViewerHandler2
    {
    public:
        ~ImageViewerHandler2();

        ImageViewWidget image_view;

        std::string equation;
        float progress = 0;

        std::shared_ptr<std::thread> wip_thread;

        // Products
        products::ImageProducts *products;

        // The Rest
        void init();
        void drawMenu();
        void drawContents(ImVec2 win_size);
        float drawTreeMenu();

        static std::string getID() { return "image_handler"; }
        static std::shared_ptr<ViewerHandler2> getInstance() { return std::make_shared<ImageViewerHandler2>(); }
    };
}