#pragma once

namespace satdump
{
    namespace firstparty
    {
        enum firstparty_product_type_t
        {
            // Invalid
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

            // AWS
            NETCDF_AWS_MWR,

            // MTG
            NETCDF_MTG_FCI,
            NETCDF_MTG_LI,

            // GOES
            NETCDF_GOES_ABI,

            // GK-2A
            NETCDF_GK2A_AMI,

            // Himawari
            HSD_HIMAWARI_AHI,

            // FengYun-2
            HDF_FY2_IMAGER,

            // FengYun-3
            HDF_FY3_MERSI,

            // GPM
            HDF_GPM_GMI,
            HDF_DMSP_SSMIS,

            // HRIT
            HRIT_GENERIC,
        };
    } // namespace firstparty
} // namespace satdump