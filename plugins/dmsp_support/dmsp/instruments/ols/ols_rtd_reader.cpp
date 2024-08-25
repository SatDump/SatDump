#include "ols_rtd_reader.h"
#include <ctime>

namespace dmsp
{
    namespace ols
    {
        OLSRTDReader::OLSRTDReader()
        {
            width = 490 * 15;

            vis_ch.resize(width);
            ir_ch.resize(width);

            lines = 0;
        }

        OLSRTDReader::~OLSRTDReader()
        {
            vis_ch.clear();
            ir_ch.clear();
        }

        void OLSRTDReader::work(uint8_t *rtd_frame, uint8_t *rtd_words)
        {
            int alarm_match = 0;

            for (int i = 1; i < 13; i++)
            {
                // Determine odd/even alarm code for word
                int alarm_code = ((i & 1) == 0) ? 0x07 : 0xFB;

                // Read word to check against alarm code
                int check_word = rtd_words[i];

                // Compare each bit and count matches
                for (int j = 1; j < 9; j++)
                {
                    if ((check_word & 0x01) == (alarm_code & 0x01))
                    {
                        alarm_match++;
                    }
                    check_word = check_word >> 1;
                    alarm_code = alarm_code >> 1;
                }

            }
            if (alarm_match > 70)
            {
                current_direction = (rtd_words[15] >> 2) & 1;
                line_sync_code = (rtd_words[13] & 0b11111100) | rtd_words[14] >> 6;
                current_pos = 0;
                lines++;
            }
            else if (rtd_words[1] == 0x00 &&
                     rtd_words[2] == 0x00)
            {
            }
            else
            {
                // Get current channels configuration
                bool tag_bit = true;
                switch (tag_bit_override)
                {
                    case 0: tag_bit = (rtd_frame[1] >> 2) & 1;
                        break;
                    case 1: tag_bit = false;
                        break;
                    case 2: tag_bit = true;
                        break;
                }

                // Extract fine pixel data
                for (int i = 0; i < 15; i++)
                    fine_channel_px[i] = rtd_words[1 + i] & 0b11111100;

                // Extract smooth pixel data
                for (int i = 0; i < 5; i++)
                    smooth_channel_px[i] = (rtd_words[1] & 3) << 6 | (rtd_words[2] & 3) << 4 | (rtd_words[3] & 3) << 2 | (rtd_words[4] & 3);
                for (int i = 0; i < 5; i++)
                    smooth_channel_px[5 + i] = (rtd_words[6] & 3) << 6 | (rtd_words[7] & 3) << 4 | (rtd_words[8] & 3) << 2 | (rtd_words[9] & 3);
                for (int i = 0; i < 5; i++)
                    smooth_channel_px[10 + i] = (rtd_words[11] & 3) << 6 | (rtd_words[12] & 3) << 4 | (rtd_words[13] & 3) << 2 | (rtd_words[14] & 3);

                if (current_pos < 490)
                {
                    if (current_direction == 0)
                    {
                        for (int i = 0; i < 15; i++)
                        {
                            int px = current_pos * 15 + i - (line_sync_code / 10);
                            if (px > 0 && px < width)
                            {
                                vis_ch[lines * width + px] = tag_bit ? smooth_channel_px[i] : fine_channel_px[i];
                                ir_ch[lines * width + px] = tag_bit ? fine_channel_px[i] : smooth_channel_px[i];
                            }
                        }
                    }
                    else if (current_direction != 0)
                    {
                        for (int i = 0; i < 15; i++)
                        {
                            int px1 = current_pos * 15 - i - (line_sync_code / 10) + offset_vis;
                            int px2 = current_pos * 15 - i - (line_sync_code / 10) + offset_ir;
                            if (px1 > 0 && px1 < width)
                                vis_ch[lines * width + ((width - 1) - px1)] = tag_bit ? smooth_channel_px[14 - i] : fine_channel_px[14 - i];
                            if (px2 > 0 && px2 < width)
                                ir_ch[lines * width + ((width - 1) - px2)] = tag_bit ? fine_channel_px[14 - i] : smooth_channel_px[14 - i];
                        }
                    }
                }

                current_pos++;
            }

            vis_ch.resize((lines + 1) * width);
            ir_ch.resize((lines + 1) * width);
        }

        image::Image OLSRTDReader::getChannelVIS()
        {
            image::Image img(vis_ch.data(), 8, width, lines, 1);
            img.mirror(true, false);
            return img;
        }

        image::Image OLSRTDReader::getChannelIR()
        {
            image::Image img(ir_ch.data(), 2, width, lines, 1);
            img.mirror(true, false);
            return img;
        }
    } // namespace avhrr
} // namespace noaa
