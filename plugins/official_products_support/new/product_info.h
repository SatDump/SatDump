#pragma once

#include "processors/processor.h"
#include "type.h"
#include "utils/file/file_iterators.h"
#include <functional>
#include <memory>
#include <string>

namespace satdump
{
    namespace official
    {
        struct OfficialProductInfo
        {
            official_product_type_t type = PRODUCT_NONE;
            std::string name;
            double timestamp;

            std::string group_id;
        };

        struct RegisteredOfficialProduct
        {
            official_product_type_t type;
            std::function<OfficialProductInfo(std::unique_ptr<satdump::utils::FilesIteratorItem> &)> testFile;
            std::function<std::shared_ptr<OfficialProductProcessor>()> getProcessor;
        };

        std::vector<RegisteredOfficialProduct> getRegisteredProducts();

        OfficialProductInfo parseOfficialInfo(std::unique_ptr<satdump::utils::FilesIteratorItem> &f);

        std::shared_ptr<OfficialProductProcessor> getProcessorForProduct(OfficialProductInfo info);
    } // namespace official
} // namespace satdump