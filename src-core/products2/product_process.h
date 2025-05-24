#pragma once

#include "product.h"

namespace satdump
{
    namespace products
    {
        void process_product_with_handler(std::shared_ptr<Product> p, std::string directory);

        inline void process_product_with_handler(Product *p, std::string directory)
        {
            satdump::products::process_product_with_handler(std::shared_ptr<satdump::products::Product>(p, [](auto *) {}), directory);
        }
    } // namespace products
} // namespace satdump