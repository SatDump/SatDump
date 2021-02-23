#include "avhrr_reader.h"

namespace noaa
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i] = new unsigned short[10000 * 2048];
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
                    uint16_t pixel = buffer[750 + channel + i * 5];
                    channels[channel][lines * 2048 + i] = pixel * 60;
                }
            }

            // Frame counter
            lines++;
        }

        cimg_library::CImg<unsigned short> AVHRRReader::getChannel(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel], 2048, lines);
        }
    } // namespace avhrr
} // namespace noaa