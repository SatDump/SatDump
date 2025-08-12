#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace firstparty
    {
        class HIRSNatProcessor : public FirstPartyProductProcessor
        {
        private:
            std::shared_ptr<satdump::products::ImageProduct> hirs_products;

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace firstparty
} // namespace satdump