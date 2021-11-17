#pragma once

#include <map>
#include <string>

namespace goes
{
    namespace grb
    {
        namespace products
        {
            namespace GLM
            {
                enum GRBGLMProductType
                {
                    META,
                    FLASH,
                    GROUP,
                    EVENT
                };

                extern std::map<int, GRBGLMProductType> GLM_PRODUCTS;
            }
        }
    }
}