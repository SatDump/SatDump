#include "ocm_reader.h"
#include "modules/metop/instruments/iasi/utils.h"

namespace oceansat
{
    namespace ocm
    {
        OCMReader::OCMReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i] = new unsigned short[10000 * 6020];
            lines = 0;
        }

        OCMReader::~OCMReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }

        void OCMReader::work(uint8_t *buffer)
        {
            // This is VERY bad... Gotta rewrite later.
            for (int i = 0; i < 6020; i++)
            {
                std::vector<uint16_t> channelss = repackBits(&buffer[1889 + i * 15], 12, 5 + 12, 12 * 7);
                channels[0][lines * 6020 + i] = channelss[0] * 10;
                channels[1][lines * 6020 + i] = channelss[1] * 10;
                channels[2][lines * 6020 + i] = channelss[2] * 10;
                channels[3][lines * 6020 + i] = channelss[3] * 10;
                channels[4][lines * 6020 + i] = channelss[4] * 10;
                channels[5][lines * 6020 + i] = channelss[5] * 10;
            }
            // channels[channel][lines * 2048 + i] = pixel * 60;

            // Frame counter
            lines++;
        }

        cimg_library::CImg<unsigned short> OCMReader::getChannel(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel], 6020, lines);
        }
    } // namespace avhrr
} // namespace noaa