#pragma once

#include "common/image/image.h"

namespace goes
{
    namespace gvar
    {
        image::Image cropIR(image::Image input);

        image::Image cropVIS(image::Image input);
    };
};