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

            struct ImageSegmentationIdentification
            {
                static constexpr int TYPE = 128;

                uint8_t type;
                uint16_t record_length;
                uint8_t image_seq_nb;
                uint8_t total_segments_nb;
                uint16_t line_nb;

                ImageSegmentationIdentification(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    image_seq_nb = data[3];
                    total_segments_nb = data[4];
                    line_nb = data[5] << 8 | data[6];
                }
            };
        } // namespace gk2a
    } // namespace xrit
} // namespace satdump