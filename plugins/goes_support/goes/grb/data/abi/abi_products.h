#pragma once

#include <map>
#include <string>

namespace goes
{
    namespace grb
    {
        namespace products
        {
            namespace ABI
            {
                // Meso Size
#define ABI_MESO_MODE_WIDTH 1000
#define ABI_MESO_MODE_HEIGHT 1000

                // CONUS Size
#define ABI_CONUS_MODE_WIDTH 5000
#define ABI_CONUS_MODE_HEIGHT 3000

                // FULLDISK Size
#define ABI_FULLDISK_MODE_WIDTH 10848
#define ABI_FULLDISK_MODE_HEIGHT 10848

                enum ABIScanType
                {
                    FULL_DISK,
                    CONUS,
                    MESO_1,
                    MESO_2
                };

                struct GRBProductABI
                {
                    int abi_mode;
                    ABIScanType type;
                    int channel;
                };

                struct ABIChannelParameters
                {
                    double resolution;
                    int bit_depth;
                };

                extern std::map<int, GRBProductABI> ABI_IMAGE_PRODUCTS;
                extern std::map<int, GRBProductABI> ABI_IMAGE_PRODUCTS_META;
                extern std::map<int, ABIChannelParameters> ABI_CHANNEL_PARAMS;

                std::string abiZoneToString(ABIScanType type);
            }
        }
    }
}