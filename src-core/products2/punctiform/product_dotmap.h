#pragma once

/**
 * @file product_dotmap.h
 */

#include "common/image/image.h"
#include "products2/punctiform_product.h"

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
    }
}