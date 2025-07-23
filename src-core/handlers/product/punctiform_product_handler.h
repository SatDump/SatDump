#pragma once

#include "handlers/image/image_handler.h"
#include "product_handler.h"
#include "products/punctiform_product.h"

namespace satdump
{
    namespace handlers
    {
        class PunctiformProductHandler : public ProductHandler
        {
        public:
            PunctiformProductHandler(std::shared_ptr<products::Product> p, bool dataset_mode = false);
            ~PunctiformProductHandler();

            // Products
            products::PunctiformProduct *product;

            // Needed for local stuff
            enum current_mode_t
            {
                MODE_GRAPH,
                MODE_DOTMAP,
                MODE_FILLMAP,
            };

            current_mode_t current_mode = MODE_GRAPH;
            float progress = 0;

            ImageHandler img_handler;

            std::string selected_channel;
            double range_min = 0;
            double range_max = 10;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            //
            void addSubHandler(std::shared_ptr<Handler> handler, bool ontop = false) { img_handler.addSubHandler(handler, ontop); }

            void delSubHandler(std::shared_ptr<Handler> handler, bool now = false) { img_handler.delSubHandler(handler, true); }

            void drawTreeMenu(std::shared_ptr<Handler> &h) { img_handler.drawTreeMenu(h); }

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            void saveResult(std::string directory);

            std::string getID() { return "punctiform_product_handler"; }
        };
    } // namespace handlers
} // namespace satdump