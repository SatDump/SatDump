#pragma once

#include "../handler.h"

namespace satdump
{
    namespace viewer
    {
        class DatasetHandler : public Handler
        {
        public:
            ~DatasetHandler();

            std::shared_ptr<Handler> instrument_products;
            std::shared_ptr<Handler> general_products;

            // The Rest
            void init();
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Dataset Test TODOREWORK"; }

            static std::string getID() { return "dataset_handler"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<DatasetHandler>(); }
        };
    }
}