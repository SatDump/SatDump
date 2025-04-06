#pragma once

#include "product_handler.h"

#include "logger.h"
#include "products2/punctiform_product.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"

namespace satdump
{
    namespace viewer
    {
        class PunctiformProductHandler : public ProductHandler
        {
        public:
            PunctiformProductHandler(std::shared_ptr<products::Product> p);
            ~PunctiformProductHandler();

            // Products
            products::PunctiformProduct *product;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            void setConfig(nlohmann::json p);
            nlohmann::json getConfig();

            void saveResult(std::string directory);

            std::string getID() { return "punctiform_product_handler"; }
        };
    }
}