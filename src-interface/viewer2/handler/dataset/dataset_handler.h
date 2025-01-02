#pragma once

/**
 * @file dataset_handler.h
 */

#include "../handler.h"

namespace satdump
{
    namespace viewer
    {
        /**
         * @brief Dataset handler.
         *
         * This is intended to handle instrument product datasets,
         * usually from a single satellite with one or more instruments.
         * It will organize things a bit better better by grouping instruments
         * and locking their handlers in place TODOREWORK, as well as setup
         * a product generator capable of processing the dataset as a whole.
         *
         * @param instrument_products handler used to store instrument products
         * @param general_products handler used to generate dataset-based products
         */
        class DatasetHandler : public Handler
        {
        public:
            DatasetHandler();
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