#pragma once

#include "codes/codes.h"
#include <cstdint>

namespace satdump
{
    namespace labrador
    {
        // TODOREWORK document all this
        template <typename T>
        bool decode_ms(code_params_t c, T *llrs, uint8_t *output, T *working, uint8_t *working_u8, uint64_t maxiters, uint64_t *fiters);
    } // namespace labrador
} // namespace satdump