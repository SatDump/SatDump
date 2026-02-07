#pragma once

#include "products/image_product.h"
namespace jpss
{
    namespace atms
    {
        inline void add_pols(satdump::products::ImageProduct *pro)
        {
            pro->set_channel_polarization(0, satdump::products::ImageProduct::POL_VERTICAL);
            pro->set_channel_polarization(1, satdump::products::ImageProduct::POL_VERTICAL);
            pro->set_channel_polarization(2, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(3, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(4, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(5, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(6, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(7, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(8, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(9, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(10, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(11, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(12, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(13, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(14, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(15, satdump::products::ImageProduct::POL_VERTICAL);
            pro->set_channel_polarization(16, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(17, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(18, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(19, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(20, satdump::products::ImageProduct::POL_HORIZONTAL);
            pro->set_channel_polarization(21, satdump::products::ImageProduct::POL_HORIZONTAL);
        }
    } // namespace atms
} // namespace jpss