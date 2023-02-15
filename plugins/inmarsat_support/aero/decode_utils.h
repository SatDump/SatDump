#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

namespace inmarsat
{
    namespace aero
    {
        void deinterleave(int8_t *input, int8_t *output, int cols);
    }
}