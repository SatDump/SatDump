#include "cpr_reader.h"
#include "common/repack.h"

#include "logger.h"

namespace cloudsat
{
    namespace cpr
    {
        CPReader::CPReader()
        {
            image = new unsigned short[1000000 * 126];
            lines = 0;
        }

        CPReader::~CPReader()
        {
            delete[] image;
        }

        void CPReader::work(uint8_t *buffer)
        {

            int out = repackBytesTo20bits(&buffer[83], 402 - 83, buffer20);

            for (int i = 0; i < 126; i++)
            {
                //logger->info(buffer20[i]);
                //uint16_t pixel = buffer[pos + channel + i * 5];
                image[lines * 126 + i] = buffer20[i] >> 4;
            }

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> CPReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(image, 126, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa