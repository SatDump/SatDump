#include "glm_products.h"

namespace goes
{
    namespace grb
    {
        namespace products
        {
            namespace GLM
            {
                std::map<int, GRBGLMProductType> GLM_PRODUCTS = {
                    {0x300, META},
                    {0x301, EVENT},
                    {0x302, FLASH},
                    {0x303, GROUP},
                };
            }
        }
    }
}