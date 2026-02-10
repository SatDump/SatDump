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

            pro->set_channel_bandwidth(0, 270e6);
            pro->set_channel_bandwidth(1, 180e6);
            pro->set_channel_bandwidth(2, 180e6);
            pro->set_channel_bandwidth(3, 400e6);
            pro->set_channel_bandwidth(4, 400e6);
            pro->set_channel_bandwidth(5, 170e6);
            pro->set_channel_bandwidth(6, 400e6);
            pro->set_channel_bandwidth(7, 400e6);
            pro->set_channel_bandwidth(8, 330e6);
            pro->set_channel_bandwidth(9, 330e6);
            pro->set_channel_bandwidth(10, 78e6);
            pro->set_channel_bandwidth(11, 36e6);
            pro->set_channel_bandwidth(12, 16e6);
            pro->set_channel_bandwidth(13, 8e3);
            pro->set_channel_bandwidth(14, 3e6);
            pro->set_channel_bandwidth(15, 2000e6);
            pro->set_channel_bandwidth(16, 3000e6);
            pro->set_channel_bandwidth(17, 2000e6);
            pro->set_channel_bandwidth(18, 2000e6);
            pro->set_channel_bandwidth(19, 1000e6);
            pro->set_channel_bandwidth(20, 1000e6);
            pro->set_channel_bandwidth(21, 500e6);
        }
    } // namespace atms
} // namespace jpss