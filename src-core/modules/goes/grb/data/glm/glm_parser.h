#pragma once

#include <string>
#include "glm_products.h"

namespace goes
{
    namespace grb
    {
        std::string parseGLMFrameToJSON(uint8_t *data, int size, products::GLM::GRBGLMProductType type);
    }
}