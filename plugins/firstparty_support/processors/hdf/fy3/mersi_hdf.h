#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace firstparty
    {
        class MERSIHdfProcessor : public FirstPartyProductProcessor
        {
        private:
            std::shared_ptr<satdump::products::ImageProduct> mersi_products;

            enum mersi_type_t
            {
                MERSI_NONE,
                MERSI_1,
                MERSI_2,
                MERSI_LL,
                MERSI_RM,
                MERSI_3,
            };

            mersi_type_t mersi_type;

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace firstparty
} // namespace satdump