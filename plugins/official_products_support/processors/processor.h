#pragma once

#include "products/product.h"
#include <cstdint>
#include <vector>

namespace satdump
{
    namespace official
    {
        class OfficialProductProcessor
        {
        public:
            virtual void ingestFile(std::vector<uint8_t> file) = 0;
            virtual std::shared_ptr<satdump::products::Product> getProduct() = 0;
        };
    } // namespace official
} // namespace satdump