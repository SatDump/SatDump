#pragma once

#include "handler.h"
#include "products2/product.h"

namespace satdump
{
    namespace viewer
    {
        class ProductHandler : public Handler
        {
        public:
            std::shared_ptr<products::Product> product;
            nlohmann::ordered_json instrument_cfg;

            static std::string getID();
            static std::shared_ptr<Handler> getInstance();
        };
    }
}