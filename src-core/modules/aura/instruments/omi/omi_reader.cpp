#include "omi_reader.h"
#include "logger.h"
namespace aura
{
    namespace omi
    {
        OMIReader::OMIReader()
        {
            for (int i = 0; i < 3; i++)
                channels[i] = new unsigned short[10000 * 2047 * 28];

            lines = 0;
        }

        OMIReader::~OMIReader()
        {
            for (int i = 0; i < 3; i++)
                delete[] channels[i];
        }

        void OMIReader::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 4116)
                return;

            int counter = packet.payload[9] & 0b11111;

            logger->warn(counter);

            if (counter == 0)
                lines++;

            for (int i = 0; i < 2047; i++)
            {
                uint16_t value = packet.payload[18 + i * 2 + 0] << 8 | packet.payload[18 + i * 2 + 1];
                channels[0][lines * 2047 * 28 + counter * 2047 + i] = value;
            }
        }

        cimg_library::CImg<unsigned short> OMIReader::getImage(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel], 2047 * 28, lines);
        }
    } // namespace ceres
} // namespace aqua