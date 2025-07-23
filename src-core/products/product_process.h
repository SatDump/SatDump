#pragma once

/**
 * @file product_process.h
 */

#include "product.h"

namespace satdump
{
    namespace products
    {
        /**
         * @brief Attempt to automatically process a product with
         * available pre-sets, such as composites and so on.
         *
         * @param p the product, loaded with loadProduct
         * @param directory output directory
         */
        void process_product_with_handler(std::shared_ptr<Product> p, std::string directory);

        /**
         * @brief Same as the process_product_with_handler, but
         * instead taking a raw pointer as an input
         *
         * @param p the product pointer
         * @param directory output directory
         */
        inline void process_product_with_handler(Product *p, std::string directory)
        {
            satdump::products::process_product_with_handler(std::shared_ptr<satdump::products::Product>(p, [](auto *) {}), directory);
        }
    } // namespace products
} // namespace satdump