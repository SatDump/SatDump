#pragma once

#include "products2/image_products.h"

namespace satdump
{
    namespace products
    {
        image::Image generate_equation_product_composite(ImageProducts *products, std::string equation, float *progess = nullptr);
    }
}