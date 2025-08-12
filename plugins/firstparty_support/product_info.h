#pragma once

#include "processors/processor.h"
#include "type.h"
#include "utils/file/file_iterators.h"
#include <functional>
#include <memory>
#include <string>

namespace satdump
{
    namespace firstparty
    {
        struct FirstPartyProductInfo
        {
            firstparty_product_type_t type = PRODUCT_NONE;
            std::string name;
            double timestamp;

            std::string group_id;
        };

        struct RegisteredFirstPartyProduct
        {
            firstparty_product_type_t type;
            std::function<FirstPartyProductInfo(std::shared_ptr<satdump::utils::FilesIteratorItem> &)> testFile;
            std::function<std::shared_ptr<FirstPartyProductProcessor>()> getProcessor;
        };

        std::vector<RegisteredFirstPartyProduct> getRegisteredProducts();

        FirstPartyProductInfo parseFirstPartyInfo(std::shared_ptr<satdump::utils::FilesIteratorItem> &f);

        std::shared_ptr<FirstPartyProductProcessor> getProcessorForProduct(FirstPartyProductInfo info);
    } // namespace firstparty
} // namespace satdump