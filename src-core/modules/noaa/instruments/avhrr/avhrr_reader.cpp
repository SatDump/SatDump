#include "avhrr_reader.h"

namespace noaa
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader(bool gac) : gac_mode(gac), width(gac_mode ? 409 : 2048)
        {
            for (int i = 0; i < 5; i++)
                channels[i] = new unsigned short[(gac_mode ? 40000 : 14000) * width];
            lines = 0;
        }

        AVHRRReader::~AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }

        void AVHRRReader::work(uint16_t *buffer)
        {
            int pos = gac_mode ? 1182 : 750; // AVHRR Data

            for (int channel = 0; channel < 5; channel++)
            {
                for (int i = 0; i < width; i++)
                {
                    uint16_t pixel = buffer[pos + channel + i * 5];
                    channels[channel][lines * width + i] = pixel * 60;
                }
            }

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> AVHRRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], width, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa
