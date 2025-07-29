#pragma once

/**
 * @file gk2a_headers.h
 * @brief GK-2A HRIT/LRIT Specific Headers
 */

#include "common/utils.h"
#include <cstdint>
#include <cstring>
#include <vector>

namespace satdump
{
    namespace xrit
    {
        namespace gk2a
        {
            enum CustomFileParams
            {
                JPG_COMPRESSED,
                J2K_COMPRESSED,
                IS_ENCRYPTED,
                KEY_INDEX,
            };

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
        } // namespace gk2a
    } // namespace xrit
} // namespace satdump