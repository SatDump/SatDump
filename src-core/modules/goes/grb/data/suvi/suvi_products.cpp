#include "suvi_products.h"

namespace goes
{
    namespace grb
    {
        namespace products
        {
            namespace SUVI
            {
                std::map<int, GRBProductSUVI> SUVI_IMAGE_PRODUCTS = {
                    // SUVI Channels
                    {0x486, {"Fe094"}},
                    {0x487, {"Fe132"}},
                    {0x488, {"Fe171"}},
                    {0x489, {"Fe195"}},
                    {0x48A, {"Fe284"}},
                    {0x48B, {"Fe304"}},
                };

                std::map<int, std::string> SUVI_IMAGE_PRODUCTS_META = {
                    {0x480, "Fe094"},
                    {0x481, "Fe132"},
                    {0x482, "Fe171"},
                    {0x483, "Fe195"},
                    {0x484, "Fe284"},
                    {0x485, "Fe304"},
                };
            }
        }
    }
}