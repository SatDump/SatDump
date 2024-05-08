#include "omi_reader.h"
#include <cstring>

namespace aura
{
    namespace omi
    {
        OMIReader::OMIReader()
        {
            for (int i = 0; i < 792; i++)
                channels[i].resize(65);
            channelRaw.resize(2047 * 28);
            visibleChannel.resize(2 * 60 * 2);

            lines = 0;
        }

        OMIReader::~OMIReader()
        {
            for (int i = 0; i < 792; i++)
                channels[i].clear();
            channelRaw.clear();
            visibleChannel.clear();
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
                    for (int c = 0; c < 792; c++)
                        channels[c][lines * 65 + i] = frameBuffer[i * 792 + c];

                lines++;
            }

            for (int i = 0; i < 2047; i++)
                frameBuffer[counter * 2047 + i] = packet.payload[18 + i * 2 + 0] << 8 | packet.payload[18 + i * 2 + 1];

            for (int i = 0; i < 792; i++)
                channels[i].resize((lines + 1) * 65);
            channelRaw.resize((lines + 1) * 2047 * 28);
            visibleChannel.resize((lines * 2 + 2) * 60 * 2);
        }

        image2::Image OMIReader::getChannel(int channel)
        {
            return image2::Image(channels[channel].data(), 16, 65, lines, 1);
        }

        image2::Image OMIReader::getImageRaw()
        {
            return image2::Image(channelRaw.data(), 16, 2047 * 28, lines, 1);
        }

        image2::Image OMIReader::getImageVisible()
        {
            return image2::Image(visibleChannel.data(), 16, 60 * 2, lines, 1);
        }
    } // namespace ceres
} // namespace aqua