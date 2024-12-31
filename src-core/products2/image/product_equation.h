#pragma once

/**
 * @file product_equation.h
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
         * @param equation equation string to generate the composite with
         * @param progess option, float progress info
         * @return generated image, with metadata (TODOREWORK)
         */
        image::Image generate_equation_product_composite(ImageProduct *product, std::string equation, float *progess = nullptr);
    }
}