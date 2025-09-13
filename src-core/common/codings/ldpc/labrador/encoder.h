#pragma once

#include "codes/codes.h"
#include <cstdint>

namespace satdump
{
    namespace labrador
    {
        // TODOREWORK document all this
        void encode_parity_u8(code_params_t c, uint8_t *data);

        void encode_parity_u32(code_params_t c, uint8_t *data);
    } // namespace labrador
} // namespace satdump