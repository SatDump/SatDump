#include "omi_reader.h"

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

        void OMIReader::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
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

        cimg_library::CImg<unsigned short> OMIReader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> img(channels[channel], 65, lines);
            img.equalize(1000);
            img.normalize(0, 65535);
            return img;
        }

        cimg_library::CImg<unsigned short> OMIReader::getImageRaw()
        {
            return cimg_library::CImg<unsigned short>(channelRaw, 2047 * 28, lines);
        }

        cimg_library::CImg<unsigned short> OMIReader::getImageVisible()
        {
            cimg_library::CImg<unsigned short> img(visibleChannel, 60 * 2, lines);
            img.equalize(1000);
            img.normalize(0, 65535);
            return img;
        }
    } // namespace ceres
} // namespace aqua