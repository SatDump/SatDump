#include "mtvza_reader.h"

namespace meteor
{
    namespace mtvza
    {
        MTVZAReader::MTVZAReader()
        {
            for (int i = 0; i < 30; i++)
                channels[i].resize(100 * 2);
            lines = 0;
        }

        MTVZAReader::~MTVZAReader()
        {
            for (int i = 0; i < 30; i++)
                channels[i].clear();
        }

        void MTVZAReader::parse_samples(uint8_t *data, int ch_start, int offset, int ch_cnt, int nsamples, int counter)
        {
            for (int ch = 0; ch < ch_cnt; ch++)
            {
                for (int i = 0; i < 4; i++)
                {
                    int sample_position = ch * nsamples + offset;
                    if (nsamples == 2)
                        sample_position += i / 2;
                    else if (nsamples == 4)
                        sample_position += i;

                    channels[ch_start + ch][lines * 100 + counter * 8 + 0 + i] = (data[8 + sample_position * 2 + (endian_mode ? 0 : 1)] << 8 | data[8 + sample_position * 2 + (endian_mode ? 1 : 0)]) - 32768;
                    channels[ch_start + ch][lines * 100 + counter * 8 + 4 + i] = (data[128 + sample_position * 2 + (endian_mode ? 0 : 1)] << 8 | data[128 + sample_position * 2 + (endian_mode ? 1 : 0)]) - 32768;
                }
            }
        }

        void MTVZAReader::work(uint8_t *data)
        {
            if (data[endian_mode ? 5 : 4] != 255)
                return;

            int counter = data[endian_mode ? 4 : 5]; // Counter, scan position

            if (counter > 26 || counter < 2)
                return;

            parse_samples(data, 0, 0, 5, 1, counter - 2);   // Parse 5 low-resolution channels
            parse_samples(data, 5, 5, 2, 4, counter - 2);   // Parse 2 full-resolution channels
            parse_samples(data, 7, 13, 23, 2, counter - 2); // Parse 23 medium-resolution channels

            // printf("%d\n", counter);

            // Frame counter
            if (counter == 26)
            {
                timestamps.push_back(latest_msumr_timestamp);
                lines++;
            }

            for (int channel = 0; channel < 30; channel++)
                channels[channel].resize((lines + 2) * 200);
        }

        image2::Image MTVZAReader::getChannel(int channel)
        {
            return image2::Image(channels[channel].data(), 16, 100, lines, 1);
        }
    }
}