#pragma once

#include <string>
#include "glm_products.h"
#include <cstdint>
namespace goes
{
    namespace grb
    {
        std::string parseGLMFrameToJSON(uint8_t *data, int size, products::GLM::GRBGLMProductType type);
    }
}
