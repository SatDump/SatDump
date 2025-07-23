#pragma once

/**
 * @file dataset_handler.h
 */

#include "../handler.h"
#include "products/dataset.h"
#include "products/product.h"

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief Dataset handler.
         *
         * This is intended to handle instrument product datasets,
         * usually from a single satellite with one or more instruments.
         * It will organize things a bit better better by grouping instruments
         * and locking their handlers in place, as well as setup a product
         * generator capable of processing the dataset as a whole.
         *
         * @param instrument_products handler used to store instrument products
         * @param general_products handler used to generate dataset-based products
         * @param dataset the dataset struct
         */
        class DatasetHandler : public Handler
        {
        private:
            products::DataSet dataset;
            std::string dataset_name = "Invalid Dataset!";

        public:
            /**
             * @brief Constructor
             * @param path path of the dataset folder
             * @param d dataset struct
             */
            DatasetHandler(std::string path, products::DataSet d);
            ~DatasetHandler();

            std::shared_ptr<Handler> instrument_products;
            std::shared_ptr<Handler> general_products;
            std::vector<std::shared_ptr<products::Product>> all_products;

            // The Rest
            void init();
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return dataset_name; }

            std::string getID() { return "dataset_handler"; }
        };
    } // namespace handlers
} // namespace satdump