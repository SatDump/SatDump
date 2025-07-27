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
            NATIVE_METOP_MHS,
            NATIVE_METOP_AMSUA,
            NATIVE_METOP_HIRS,
            NATIVE_METOP_IASI,
            NATIVE_METOP_GOME,

            // MTG
            NETCDF_MTG_FCI,
            NETCDF_MTG_LI,

            // GOES
            NETCDF_GOES_ABI,

            // GK-2A
            NETCDF_GK2A_AMI,

            // Himawari
            HSD_HIMAWARI_AHI,

            // HRIT
            HRIT_GENERIC,
        };
    } // namespace official
} // namespace satdump