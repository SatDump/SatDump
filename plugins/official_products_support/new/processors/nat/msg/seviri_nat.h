#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace official
    {
        class SEVIRINatProcessor : public OfficialProductProcessor
        {
        private:
            std::shared_ptr<satdump::products::ImageProduct> seviri_products;

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace official
} // namespace satdump