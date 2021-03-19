#include "ocm_reader.h"
#include "modules/metop/instruments/iasi/utils.h"

namespace oceansat
{
    namespace ocm
    {
        OCMReader::OCMReader()
        {
            for (int i = 0; i < 8; i++)
                channels[i] = new unsigned short[10000 * 4072];
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
            // This is VERY bad... Gotta rewrite later.
            /* for (int i = 0; i < 4072; i++)
            {
                std::vector<uint16_t> channelss = repackBits(&buffer[1889 + (i + 964) * 15], 12, 5 + 12, 12 * 9);
                channels[0][lines * 4072 + i] = channelss[0] * 12;
                channels[1][lines * 4072 + i] = channelss[1] * 12;
                channels[2][lines * 4072 + i] = channelss[2] * 12;
                channels[3][lines * 4072 + i] = channelss[3] * 12;
                channels[4][lines * 4072 + i] = channelss[4] * 12;
                channels[5][lines * 4072 + i] = channelss[5] * 12;
                channels[6][lines * 4072 + i] = channelss[6] * 12;
                channels[7][lines * 4072 + i] = channelss[7] * 12;
            }
            // channels[channel][lines * 2048 + i] = pixel * 60;
*/

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
                channels[0][lines * 4072 + i] = lineBuffer[i * 10 + 0] * 12;
                channels[1][lines * 4072 + i] = lineBuffer[i * 10 + 1] * 12;
                channels[2][lines * 4072 + i] = lineBuffer[i * 10 + 2] * 12;
                channels[3][lines * 4072 + i] = lineBuffer[i * 10 + 3] * 12;
                channels[4][lines * 4072 + i] = lineBuffer[i * 10 + 4] * 12;
                channels[5][lines * 4072 + i] = lineBuffer[i * 10 + 5] * 12;
                channels[6][lines * 4072 + i] = lineBuffer[i * 10 + 6] * 12;
                channels[7][lines * 4072 + i] = lineBuffer[i * 10 + 7] * 12;
            }

            // Frame counter
            lines++;
        }

        cimg_library::CImg<unsigned short> OCMReader::getChannel(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel], 4072, lines);
        }
    } // namespace avhrr
} // namespace noaa