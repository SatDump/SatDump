#pragma once

#include <cstdint>
namespace satdump
{
    namespace ndsp
    {
        // Fast inverse square root, the usual. https://en.wikipedia.org/wiki/Fast_inverse_square_root
        inline float fast_invsqrt(float x)
        {
            union {
                float f;
                uint32_t i;
            } u = {x};
            u.i = 0x5f3759dfu - (u.i >> 1);
            float y = u.f;
            return y * (1.5f - 0.5f * x * y * y);
        }
    } // namespace ndsp
} // namespace satdump