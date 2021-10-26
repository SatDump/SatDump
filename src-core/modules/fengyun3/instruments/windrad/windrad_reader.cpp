#include "windrad_reader.h"
#include "logger.h"

namespace fengyun3
{
    namespace windrad
    {
        WindRADReader::WindRADReader(int width, std::string bnd, std::string dir) : width(width), band(bnd), directory(dir)
        {
            for (int i = 0; i < 2; i++)
                channels[i].create(10000 * width);

            lines = 0;
        }

        WindRADReader::~WindRADReader()
        {
            for (int i = 0; i < 2; i++)
                channels[i].destroy();
        }

        void WindRADReader::work(std::vector<uint8_t> &packet)
        {
            if (packet.size() < 1645)
                return;

            int marker = (packet[6] >> 3) & 0b111;

            if (lastMarker == 5 && marker == 1)
            {
                logger->info("New frame, saving...");
                imgCount++;

                logger->info("Channel 1...");
                getChannel(0).save_png(std::string(directory + "/WindRAD-" + band + "1-" + std::to_string(imgCount) + ".png").c_str());

                logger->info("Channel 2...");
                getChannel(1).save_png(std::string(directory + "/WindRAD-" + band + "2-" + std::to_string(imgCount) + ".png").c_str());

                lines = 0;
            }

            lastMarker = marker;

            if (marker > 1)
                return;

            for (int i = 0; i < width; i++)
            {
                channels[marker][lines * width + i] = packet[17 + i * 2 + 0] << 8 | packet[17 + i * 2 + 1];
            }

            // Frame counter
            if (marker == 1)
                lines++;

            // Make sure we have enough room
            if (lines * width >= (int)channels[0].size())
            {
                for (int i = 0; i < 2; i++)
                    channels[i].resize((lines + 1000) * width);
            }
        }

        cimg_library::CImg<unsigned short> WindRADReader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> image(channels[channel].buf, width, lines);
            image.normalize(0, 65535);
            image.equalize(1000);
            return image;
        }
    } // namespace virr
} // namespace fengyun