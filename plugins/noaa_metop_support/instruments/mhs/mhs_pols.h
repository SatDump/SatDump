#pragma once

#include "products/image_product.h"
namespace noaa_metop
{
    namespace mhs
    {
        inline void add_pols(satdump::products::ImageProduct *pro)
        {
            pro->set_channel_polarization(0, satdump::products::ImageProduct::POL_VERTICAL);
            pro->set_channel_polarization(1, satdump::products::ImageProduct::POL_VERTICAL);
            pro->set_channel_polarization(2, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(3, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(4, satdump::products::ImageProduct::POL_VERTICAL);
        }
    } // namespace mhs
} // namespace noaa_metop