#include "ocm_reader.h"
#include <cmath>

namespace oceansat
{
    namespace ocm
    {
        OCMReader::OCMReader()
        {
            for (int i = 0; i < 8; i++)
                channels[i].resize(4072);
            lines = 0;
        }

        OCMReader::~OCMReader()
        {
            for (int i = 0; i < 8; i++)
                channels[i].clear();
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
                channels[0][lines * 4072 + i] = lineBuffer[i * 10 + 0] << 4;
                channels[1][lines * 4072 + i] = lineBuffer[i * 10 + 1] << 4;
                channels[2][lines * 4072 + i] = lineBuffer[i * 10 + 2] << 4;
                channels[3][lines * 4072 + i] = lineBuffer[i * 10 + 3] << 4;
                channels[4][lines * 4072 + i] = lineBuffer[i * 10 + 4] << 4;
                channels[5][lines * 4072 + i] = lineBuffer[i * 10 + 5] << 4;
                channels[6][lines * 4072 + i] = lineBuffer[i * 10 + 6] << 4;
                channels[7][lines * 4072 + i] = lineBuffer[i * 10 + 7] << 4;
            }

            // Frame counter
            lines++;

            for (int i = 0; i < 8; i++)
                channels[i].resize((lines + 1) * 4072);
        }

        image::Image OCMReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 4072, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa