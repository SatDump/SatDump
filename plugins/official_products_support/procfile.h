#pragma once

#include "product_info.h"
#include "products/product.h"
#include <memory>

namespace satdump
{
    namespace official
    {
        OfficialProductInfo identifyOfficialProductFile(std::string fpath);
        std::shared_ptr<products::Product> processOfficialProductFile(OfficialProductInfo info, std::string fpath);
    } // namespace official
} // namespace satdump