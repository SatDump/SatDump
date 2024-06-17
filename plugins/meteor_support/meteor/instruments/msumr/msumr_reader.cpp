#include "msumr_reader.h"

namespace meteor
{
    namespace msumr
    {
        MSUMRReader::MSUMRReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i].resize(1572);
            lines = 0;
        }

        MSUMRReader::~MSUMRReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i].clear();
        }

        void MSUMRReader::work(uint8_t *buffer)
        {
            // Convert into 10-bits values
            // 393 byte per channel
            for (int channel = 0; channel < 6; channel++)
            {
                for (int l = 0; l < 393; l++)
                {
                    int pixelpos = 50 + l * 30 + channel * 5;

                    // Convert 5 bytes to 4 10-bits values
                    channels[channel][lines * 1572 + l * 4 + 0] = ((buffer[pixelpos] << 2) | (buffer[pixelpos + 1] >> 6)) << 6;
                    channels[channel][lines * 1572 + l * 4 + 1] = (((buffer[pixelpos + 1] % 64) << 4) | (buffer[pixelpos + 2] >> 4)) << 6;
                    channels[channel][lines * 1572 + l * 4 + 2] = (((buffer[pixelpos + 2] % 16) << 6) | (buffer[pixelpos + 3] >> 2)) << 6;
                    channels[channel][lines * 1572 + l * 4 + 3] = (((buffer[pixelpos + 3] % 4) << 8) | buffer[pixelpos + 4]) << 6;
                }
            }

            // Frame counter
            lines++;

            for (int channel = 0; channel < 6; channel++)
                channels[channel].resize((lines + 1) * 1572);
        }

        image::Image MSUMRReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 1572, lines, 1);
        }
    } // namespace msumr
} // namespace meteor