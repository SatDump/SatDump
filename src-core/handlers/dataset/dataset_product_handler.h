#pragma once

#include "../handler.h"
#include "../processing_handler.h"
#include "dataset_handler.h"

namespace satdump
{
    namespace handlers
    {
        class DatasetProductHandler : public Handler
        {
        public:
            DatasetProductHandler();
            ~DatasetProductHandler();

            products::Product *get_instrument_products(std::string v, int index)
            {
                std::vector<products::Product *> pro;
                for (auto &h : dataset_handler->all_products)
                    if (h->instrument_name == v)
                        pro.push_back(h.get());
                return index < pro.size() ? pro[index] : nullptr;
            };

            DatasetHandler *dataset_handler;

            // Presets / processor list
            std::string current_cfg;
            std::vector<nlohmann::json> available_presets;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Products"; }

            std::string getID() { return "dataset_product_handler"; }
        };
    } // namespace handlers
} // namespace satdump