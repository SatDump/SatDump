#include "windrad_reader.h"
#include "logger.h"
#include <cmath>

namespace fengyun3
{
    namespace windrad
    {
        WindRADReader::WindRADReader(int width, std::string bnd, std::string dir) : width(width), band(bnd), directory(dir)
        {
            for (int i = 0; i < 2; i++)
                channels[i].create(width);

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

                if (lastMarker2 == 21)
                {
                    imgCount++;
                    getChannel(0).save_img(std::string(directory + "/WindRAD-Pol1-" + band + "-" + std::to_string(imgCount)).c_str());
                    getChannel(1).save_img(std::string(directory + "/WindRAD-Pol2-" + band + "-" + std::to_string(imgCount)).c_str());
                }
                else
                {
                    getChannel(0).save_img(std::string(directory + "/WindRAD-Pol3-" + band + "-" + std::to_string(imgCount)).c_str());
                    getChannel(1).save_img(std::string(directory + "/WindRAD-Pol4-" + band + "-" + std::to_string(imgCount)).c_str());
                }

                lines = 0;
            }

            lastMarker = marker;
            if (marker == 3)
                lastMarker2 = packet[15] >> 2;

            if (marker > 1)
                return;

            for (int i = 0; i < width; i++)
            {
                // channels[marker][lines * width + i] = packet[17 + i * 2 + 0] << 8 | packet[17 + i * 2 + 1];
                // float test = packet[17 + i * 2 + 0] << 8 | packet[17 + i * 2 + 1];
                bool sign = (packet[17 + i * 2 + 0] >> 7) & 1;
                float exponent = (packet[17 + i * 2 + 0] >> 2) & 0b11111;
                float significand = (packet[17 + i * 2 + 0] & 0b11) << 8 | packet[17 + i * 2 + 1];
                float test = (sign ? -1 : 1) * significand * powf(2, exponent - 15);

                test /= 500;

                // logger->critical("%f - S %d - E %f - S %f", test, (int)sign, exponent - 15, significand);
                if (test < 0)
                    test = 0;
                if (test > 65535)
                    test = 65535;
                channels[marker][lines * width + i] = test;
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

        image::Image<uint16_t> WindRADReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].buf, width, lines, 1);
        }
    } // namespace virr
} // namespace fengyun