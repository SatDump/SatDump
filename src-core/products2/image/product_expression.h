#pragma once

/**
 * @file product_expression.h
 */

#include "products2/image_product.h"

namespace satdump
{
    namespace products
    {
        /**
         * @brief Generate a simple composite from an image product
         * TODOREWORK : document selection of best settings. Max bit depth, size, etc
         *
         * @param product the image product to utilize
         * @param expression expression string to generate the composite with
         * @param progress option, float progress info. Special case : if -1, run in
         * dummy mode
         * @return generated image, with metadata (TODOREWORK)
         */
        image::Image generate_expression_product_composite(ImageProduct *product, std::string expression, float *progress = nullptr);

        /**
         * @brief Check if an expression can be generated.
         *
         * @param product the image product to utilize
         * @param expression expression string
         * @return true if it can be generated
         */
        bool check_expression_product_composite(ImageProduct *product, std::string expression);
    } // namespace products
} // namespace satdump