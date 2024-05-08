#include "gas_reader.h"

namespace fengyun3
{
    namespace gas
    {
        GASReader::GASReader()
        {
            imageBuffer = new unsigned short[1000 * 335202];
            lines = 0;
        }

        GASReader::~GASReader()
        {
            delete[] imageBuffer;
        }

        void GASReader::work(std::vector<uint8_t> &packet)
        {
            // if (packet.size() < 65542)
            //     return;

            // int flag = (packet[10] >> 6) & 0b11;

            // logger->info(flag);

            // if (flag == 2)
            //     return;

            for (int i = 0; i < 335202; i++)
            {
                uint16_t value = packet[4 + i * 2 + 0] << 8 | packet[4 + i * 2 + 1];
                imageBuffer[lines * 335202 + i] = value; // << 3;
            }

            // Frame counter
            lines++;
        }

        image::Image GASReader::getChannel()
        {
            return image::Image(imageBuffer, 16, 335202, lines, 1);
        }
    } // namespace virr
} // namespace fengyun