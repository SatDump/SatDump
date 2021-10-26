#include "waai_reader.h"

namespace fengyun3
{
    namespace waai
    {
        WAAIReader::WAAIReader()
        {
            imageBuffer = new unsigned short[10000 * 32737];
            lines = 0;
        }

        WAAIReader::~WAAIReader()
        {
            delete[] imageBuffer;
        }

        void WAAIReader::work(std::vector<uint8_t> &packet)
        {
            if (packet.size() < 65542)
                return;

            //int flag = (packet[10] >> 6) & 0b11;

            //logger->info(flag);

            //if (flag == 2)
            //    return;

            for (int i = 0; i < 32737; i++)
            {
                uint16_t value = packet[68 + i * 2 + 0] << 8 | packet[68 + i * 2 + 1];
                imageBuffer[lines * 32737 + i] = value; // << 3;
            }

            // Frame counter
            lines++;
        }

        cimg_library::CImg<unsigned short> WAAIReader::getChannel()
        {
            return cimg_library::CImg<unsigned short>(imageBuffer, 32737, lines);
        }
    } // namespace virr
} // namespace fengyun