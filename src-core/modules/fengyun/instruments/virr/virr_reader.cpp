#include "virr_reader.h"

namespace fengyun
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

            // Make sure we have enough room
            if (lines * 2048 >= (int)channels[0].size())
            {
                for (int i = 0; i < 10; i++)
                    channels[i].resize((lines + 1000) * 2048);
            }
        }

        cimg_library::CImg<unsigned short> VIRRReader::getChannel(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel].buf, 2048, lines);
        }
    } // namespace virr
} // namespace fengyun