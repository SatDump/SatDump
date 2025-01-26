#pragma once

#include "product.h"

namespace satdump
{
    namespace products
    {
        void process_product_with_handler(std::shared_ptr<Product> p, std::string directory);
    }
}