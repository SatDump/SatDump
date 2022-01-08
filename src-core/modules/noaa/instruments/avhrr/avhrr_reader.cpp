#include "avhrr_reader.h"

namespace noaa
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i] = new unsigned short[10000 * 2048];
            lines = 0;
        }

        AVHRRReader::~AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }

        void AVHRRReader::work(uint16_t *buffer)
        {
            int pos = 750; // AVHRR Data

            for (int channel = 0; channel < 5; channel++)
            {
                for (int i = 0; i < 2048; i++)
                {
                    uint16_t pixel = buffer[pos + channel + i * 5];
                    channels[channel][lines * 2048 + i] = pixel * 60;
                }
            }

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> AVHRRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 2048, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa