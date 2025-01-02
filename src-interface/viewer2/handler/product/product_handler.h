#pragma once

/**
 * @file product_handler.h
 */

#include "../handler.h"
#include "products2/product.h"

namespace satdump
{
    namespace viewer
    {
        /**
         * @brief Product handler base class.
         *
         * A common usecase of handlers in the viewer is to allow
         * displaying and doing basic manipulation on instrument products.
         *
         * To this effect, this base class implements the common interface
         * for handling those instrument products.
         *
         * @param product raw product pointer, to be cast in the inheriting class
         * @param instrument_cfg TODOREWORK instrument configuration file, for presets
         */
        class ProductHandler : public Handler
        {
        public:
            ProductHandler(std::shared_ptr<products::Product> p)
                : product(p) {}

            std::shared_ptr<products::Product> product;
            nlohmann::ordered_json instrument_cfg;

            static std::string getID();
            static std::shared_ptr<Handler> getInstance();
        };
    }
}