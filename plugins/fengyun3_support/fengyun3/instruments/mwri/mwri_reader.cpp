#include "mwri_reader.h"

namespace fengyun3
{
    namespace mwri
    {
        MWRIReader::MWRIReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].create(266);

            lines = 0;
        }

        MWRIReader::~MWRIReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].destroy();
        }

        void MWRIReader::work(std::vector<uint8_t> &packet)
        {
            if (packet.size() < 7546)
                return;

            for (int ch = 0; ch < 10; ch++)
            {
                for (int i = 0; i < 266; i++)
                {
                    channels[ch][lines * 266 + 265 - i] = packet[200 + ch * 727 + i * 2 + 1] << 8 | packet[200 + ch * 727 + i * 2 + 0];
                }
            }

            // Parse timestamp
            uint8_t timestamp[8];

            timestamp[0] = packet[14];
            timestamp[1] = packet[15];

            timestamp[2] = packet[16];
            timestamp[3] = packet[17];

            timestamp[4] = packet[18];
            timestamp[5] = packet[19] & 0b11110000;

            uint16_t days = timestamp[0] << 8 | timestamp[1];
            uint64_t milliseconds_of_day = timestamp[2] << 24 | timestamp[3] << 16 | timestamp[4] << 8 | timestamp[5];
            uint16_t subsecond_cnt = (packet[20] & 0b11) << 8 | packet[21];

            double currentTime = double(10957 + days) * 86400.0 +
                                 double(milliseconds_of_day) / double(1e3) +
                                 double(subsecond_cnt) / 512 + 12 * 3600;

            timestamps.push_back(currentTime);

            // Frame counter
            lines++;

            // Make sure we have enough room
            if (lines * 266 >= (int)channels[0].size())
            {
                for (int i = 0; i < 10; i++)
                    channels[i].resize((lines + 1000) * 266);
            }
        }

        image::Image<uint16_t> MWRIReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].buf, 266, lines, 1);
        }
    } // namespace virr
} // namespace fengyun