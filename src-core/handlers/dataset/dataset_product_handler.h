#pragma once

#include "../handler.h"
#include "../processing_handler.h"
#include "dataset_handler.h"

#include "processor.h"

namespace satdump
{
    namespace handlers
    {
        class DatasetProductHandler : public Handler, public ProcessingHandler
        {
        public:
            DatasetProductHandler();
            ~DatasetProductHandler();

            DatasetHandler *dataset_handler;

            // Presets / processor list
            std::string current_cfg;
            std::vector<nlohmann::json> available_presets;
            std::unique_ptr<DatasetProductProcessor> processor;

            // Proc function
            void do_process();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Products"; }

            std::string getID() { return "dataset_product_handler"; }
        };
    } // namespace handlers
} // namespace satdump