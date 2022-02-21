#include "virr_reader.h"

namespace fengyun3
{
    namespace virr
    {
        VIRRReader::VIRRReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].create(10000 * 2048);

            lines = 0;
        }

        VIRRReader::~VIRRReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].destroy();
        }

        void VIRRReader::work(std::vector<uint8_t> &packet)
        {
            if (packet.size() < 12960)
                return;

            int pos = 436; // VIRR Data position, found through a bit viewer

            // Convert into 10-bits values
            for (int i = 0; i < 20480; i += 4)
            {
                virrBuffer[i + 0] = (packet[pos + 0] & 0b111111) << 4 | packet[pos + 1] >> 4;
                virrBuffer[i + 1] = (packet[pos + 1] & 0b1111) << 6 | packet[pos + 2] >> 2;
                virrBuffer[i + 2] = (packet[pos + 2] & 0b11) << 8 | packet[pos + 3];
                virrBuffer[i + 3] = packet[pos + 4] << 2 | packet[pos + 5] >> 6;
                pos += 5;
            }

            for (int channel = 0; channel < 10; channel++)
            {
                for (int i = 0; i < 2048; i++)
                {
                    uint16_t pixel = virrBuffer[channel + i * 10];
                    channels[channel][lines * 2048 + i] = pixel * 60;
                }
            }

            // Frame counter
            lines++;

            // Parse timestamp
            {
                uint8_t timestamp[8];
                timestamp[0] = (packet[26041] & 0b111111) << 2 | packet[26042] >> 6;
                timestamp[1] = (packet[26042] & 0b111111) << 2 | packet[26043] >> 6;
                timestamp[2] = (packet[26043] & 0b111111) << 2 | packet[26044] >> 6;
                timestamp[3] = (packet[26044] & 0b111111) << 2 | packet[26045] >> 6;
                timestamp[4] = (packet[26045] & 0b111111) << 2 | packet[26046] >> 6;
                timestamp[6] = (packet[26046] & 0b111111) << 2 | packet[26047] >> 6;
                timestamp[7] = (packet[26047] & 0b111111) << 2 | packet[26048] >> 6;

                uint16_t days = (timestamp[1] & 0b11) << 10 | timestamp[2] << 2 | timestamp[3] >> 6; // Appears to be days since launch?
                uint32_t milliseconds_of_day = (timestamp[3] & 0b11) << 24 | timestamp[4] << 16 | timestamp[6] << 8 | timestamp[7];
                double currentTime = double(day_offset + days) * 86400.0 + double(milliseconds_of_day) / double(1000) + 12 * 3600;

                timestamps.push_back(currentTime);
            }

            // Make sure we have enough room
            if (lines * 2048 >= (int)channels[0].size())
            {
                for (int i = 0; i < 10; i++)
                    channels[i].resize((lines + 1000) * 2048);
            }
        }

        image::Image<uint16_t> VIRRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].buf, 2048, lines, 1);
        }
    } // namespace virr
} // namespace fengyun