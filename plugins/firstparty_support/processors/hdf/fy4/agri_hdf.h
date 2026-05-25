#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace firstparty
    {
        class AGRIHdfProcessor : public FirstPartyProductProcessor
        {
        private:
            std::shared_ptr<satdump::products::ImageProduct> agri_products;

            double sat_lon;
            double sat_height;

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace firstparty
} // namespace satdump