#pragma once

#include "product_info.h"
#include "products/product.h"
#include <memory>

namespace satdump
{
    namespace firstparty
    {
        FirstPartyProductInfo identifyFirstPartyProductFile(std::string fpath);
        std::shared_ptr<products::Product> processFirstPartyProductFile(FirstPartyProductInfo info, std::string fpath);
    } // namespace firstparty
} // namespace satdump