#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include "common/utils.h"

namespace gk2a
{
    namespace lrit
    {
        struct KeyHeader
        {
            static constexpr int TYPE = 7;

            uint8_t type;
            uint16_t record_length;
            uint32_t key;

            KeyHeader(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                key = data[3] << 24 | data[4] << 16 | data[5] << 8 | data[6];
            }
        };
    }
}