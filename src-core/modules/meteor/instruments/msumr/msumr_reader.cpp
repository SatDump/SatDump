#include "msumr_reader.h"

namespace meteor
{
    namespace msumr
    {
        MSUMRReader::MSUMRReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i] = new unsigned short[10000 * 1572];
            lines = 0;
        }

        MSUMRReader::~MSUMRReader()
        {
            for (int i = 0; i < 6; i++)
                delete[] channels[i];
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
                    msumrBuffer[channel][l * 4 + 0] = (buffer[pixelpos] << 2) | (buffer[pixelpos + 1] >> 6);
                    msumrBuffer[channel][l * 4 + 1] = ((buffer[pixelpos + 1] % 64) << 4) | (buffer[pixelpos + 2] >> 4);
                    msumrBuffer[channel][l * 4 + 2] = ((buffer[pixelpos + 2] % 16) << 6) | (buffer[pixelpos + 3] >> 2);
                    msumrBuffer[channel][l * 4 + 3] = ((buffer[pixelpos + 3] % 4) << 8) | buffer[pixelpos + 4];
                }
            }

            for (int channel = 0; channel < 6; channel++)
            {
                for (int i = 0; i < 1572; i++)
                {
                    channels[channel][lines * 1572 + i] = msumrBuffer[channel][i] * 60;
                }
            }

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> MSUMRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 1572, lines, 1);
        }
    } // namespace msumr
} // namespace meteor