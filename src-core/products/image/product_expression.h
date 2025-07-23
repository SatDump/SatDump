#pragma once

/**
 * @file product_expression.h
 */

#include "products/image_product.h"

namespace satdump
{
    namespace products
    {
        /**
         * @brief Generate a simple composite from an image product.
         * For more info see <a href="md_docs_2pages_2ImageProductExpression.html">the Image Product Expression page</a>.  
         *
         * @param product the image product to utilize
         * @param expression expression string to generate the composite with
         * @param progress option, float progress info. Special case : if -1, run in
         * dummy mode
         * @return generated image, with projection metadata 
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