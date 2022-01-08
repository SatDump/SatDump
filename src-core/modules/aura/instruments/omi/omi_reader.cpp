#include "omi_reader.h"
#include <cstring>

namespace aura
{
    namespace omi
    {
        OMIReader::OMIReader()
        {
            for (int i = 0; i < 792; i++)
                channels[i] = new unsigned short[10000 * 65];

            channelRaw = new unsigned short[10000 * 2047 * 28];

            frameBuffer = new unsigned short[2047 * 28];
            visibleChannel = new unsigned short[10000 * 60 * 2];

            lines = 0;
        }

        OMIReader::~OMIReader()
        {
            for (int i = 0; i < 792; i++)
                delete[] channels[i];

            delete[] channelRaw;
            delete[] frameBuffer;
            delete[] visibleChannel;
        }

        void OMIReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 4116)
                return;

            int counter = packet.payload[9] & 0b11111;

            if (counter > 27)
                return;

            if (counter == 0)
            {
                lines++;

                // Global
                std::memcpy(&channelRaw[lines * 2047 * 28], frameBuffer, 2047 * 28 * sizeof(unsigned short));

                // Visible? Some low res on the side
                for (int i = 0; i < 60; i++)
                {
                    visibleChannel[(lines * 2 + 0) * 60 * 2 + i * 2 + 0] = frameBuffer[51482 + i];
                    visibleChannel[(lines * 2 + 0) * 60 * 2 + i * 2 + 1] = frameBuffer[51547 + i];
                    visibleChannel[(lines * 2 + 1) * 60 * 2 + i * 2 + 0] = frameBuffer[51612 + i];
                    visibleChannel[(lines * 2 + 1) * 60 * 2 + i * 2 + 1] = frameBuffer[51677 + i];
                }

                // All individual channels
                for (int i = 0; i < 65; i++)
                {
                    for (int c = 0; c < 792; c++)
                    {
                        channels[c][lines * 65 + i] = frameBuffer[i * 792 + c];
                    }
                }
            }

            for (int i = 0; i < 2047; i++)
            {
                uint16_t value = packet.payload[18 + i * 2 + 0] << 8 | packet.payload[18 + i * 2 + 1];
                frameBuffer[counter * 2047 + i] = value;
            }
        }

        image::Image<uint16_t> OMIReader::getChannel(int channel)
        {
            image::Image<uint16_t> img(channels[channel], 65, lines, 1);
            img.equalize();
            img.normalize();
            return img;
        }

        image::Image<uint16_t> OMIReader::getImageRaw()
        {
            return image::Image<uint16_t>(channelRaw, 2047 * 28, lines, 1);
        }

        image::Image<uint16_t> OMIReader::getImageVisible()
        {
            image::Image<uint16_t> img(visibleChannel, 60 * 2, lines, 1);
            img.equalize();
            img.normalize();
            return img;
        }
    } // namespace ceres
} // namespace aqua