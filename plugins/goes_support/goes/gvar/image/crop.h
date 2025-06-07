#pragma once

#include "image/image.h"

namespace goes
{
    namespace gvar
    {
        image::Image cropIR(image::Image input);

        image::Image cropVIS(image::Image input);
    };
};