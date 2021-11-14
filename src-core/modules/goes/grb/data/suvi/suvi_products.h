#pragma once

#include <map>
#include <string>

namespace goes
{
    namespace grb
    {
        namespace products
        {
            namespace SUVI
            {
                struct GRBProductSUVI
                {
                    std::string channel;
                    int width = 1280;
                    int height = 1280;
                };

                extern std::map<int, GRBProductSUVI> SUVI_IMAGE_PRODUCTS;
                extern std::map<int, std::string> SUVI_IMAGE_PRODUCTS_META;
            }
        }
    }
}