#pragma once

#include "common/image/image.h"

namespace goes
{
    namespace gvar
    {
        template <typename T>
        image::Image<T> cropIR(image::Image<T> input);

        template <typename T>
        image::Image<T> cropVIS(image::Image<T> input);
    };
};