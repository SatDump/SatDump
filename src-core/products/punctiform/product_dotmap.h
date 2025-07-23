#pragma once

/**
 * @file product_dotmap.h
 */

#include "image/image.h"
#include "products/punctiform_product.h"

namespace satdump
{
    namespace products
    {
        /**
        TODOREWORK Make this generic, and extract points & projection from product
         */
        image::Image generate_dotmap_product_image(PunctiformProduct *product, std::string channel,
                                                   int width, int height, int dotsize, bool background, bool use_lut,
                                                   double min, double max,
                                                   float *progress = nullptr);

        /**
        TODOREWORK Make this generic, and extract points & projection from product
        */
        image::Image generate_fillmap_product_image(PunctiformProduct *product, std::string channel,
                                                    int width, int height, double min, double max,
                                                    float *progress = nullptr);
    }
}