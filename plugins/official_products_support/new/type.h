#pragma once

namespace satdump
{
    namespace official
    {
        enum official_product_type_t
        {
            // MSG
            PRODUCT_NONE,

            // MSG
            NATIVE_MSG_SEVIRI,

            // MetOp
            NATIVE_METOP_AVHRR,

            // MTG
            NETCDF_MTG_FCI,
            NETCDF_MTG_LI,

            // GOES
            NETCDF_GOES_ABI,

            // Himawari
            HSD_HIMAWARI_AHI,
        };
    } // namespace official
} // namespace satdump