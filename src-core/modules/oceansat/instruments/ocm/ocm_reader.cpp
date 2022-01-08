#include "ocm_reader.h"
#include "modules/metop/instruments/iasi/utils.h"
#include <cmath>

namespace oceansat
{
    namespace ocm
    {
        OCMReader::OCMReader()
        {
            for (int i = 0; i < 8; i++)
                channels[i] = new unsigned short[20000 * 4072];
            lineBuffer = new unsigned short[4072 * 10];
            lines = 0;
        }

        OCMReader::~OCMReader()
        {
            for (int i = 0; i < 8; i++)
                delete[] channels[i];
            delete[] lineBuffer;
        }

        void OCMReader::work(uint8_t *buffer)
        {
            int pos = 1889 + 964 * 15 + 2; // OCM Data position, found through a bit viewer

            // Convert into 12-bits values
            for (int i = 0; i < 4072 * 10; i += 2)
            {
                lineBuffer[i] = buffer[pos + 0] << 4 | buffer[pos + 1] >> 4;
                lineBuffer[i + 1] = (buffer[pos + 1] % (int)pow(2, 4)) << 8 | buffer[pos + 2];
                pos += 3;
            }

            for (int i = 0; i < 4072; i += 1)
            {
                channels[0][lines * 4072 + i] = lineBuffer[i * 10 + 0] * 16;
                channels[1][lines * 4072 + i] = lineBuffer[i * 10 + 1] * 16;
                channels[2][lines * 4072 + i] = lineBuffer[i * 10 + 2] * 16;
                channels[3][lines * 4072 + i] = lineBuffer[i * 10 + 3] * 16;
                channels[4][lines * 4072 + i] = lineBuffer[i * 10 + 4] * 16;
                channels[5][lines * 4072 + i] = lineBuffer[i * 10 + 5] * 16;
                channels[6][lines * 4072 + i] = lineBuffer[i * 10 + 6] * 16;
                channels[7][lines * 4072 + i] = lineBuffer[i * 10 + 7] * 16;
            }

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> OCMReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 4072, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa