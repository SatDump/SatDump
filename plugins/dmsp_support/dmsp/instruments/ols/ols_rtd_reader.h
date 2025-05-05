#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>

namespace dmsp
{
    namespace ols
    {
        class OLSRTDReader
        {
        private:
            std::vector<uint8_t> vis_ch;
            std::vector<uint8_t> ir_ch;

            int width;

            uint8_t fine_channel_px[15];
            uint8_t smooth_channel_px[15];

            int current_pos = 0;
            int current_direction = 0;
            int line_sync_code = 0;

            int offset_ir = 0;
            int offset_vis = 0;

            int tag_bit_override = 0;

        public:
            OLSRTDReader();
            ~OLSRTDReader();

            int lines;

            void work(uint8_t *rtd_frame, uint8_t *rtd_words);

            image::Image getChannelVIS();
            image::Image getChannelIR();

            void set_offsets(int i, int v)
            {
                offset_ir = i;
                offset_vis = v;
            }

            void set_tag_bit(int t)
            {
                tag_bit_override = t;
            }
        };
    } // namespace avhrr
} // namespace noaa