#include "ascat_reader.h"
#include "common/ccsds/ccsds_time.h"
#include <cmath>

namespace metop
{
    namespace ascat
    {
        ASCATReader::ASCATReader()
        {
            for (int i = 0; i < 6; i++)
            {
                channels_img[i].resize(256);
                lines[i] = 0;

                noise_lines[i] = 0;
            }
        }

        ASCATReader::~ASCATReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i].clear();
        }

        double parse_uint_to_float(uint16_t sample)
        {
            // Details of this format from pyascatreader.cpp
            bool s = (sample >> 15) & 0x1;
            unsigned int e = (sample >> 7) & 0xff;
            unsigned int f = (sample) & 0x7f;

            double value = 0;

            if (e == 255)
            {
                value = 0.0;
            }
            else if (e == 0)
            {
                if (f == 0)
                    value = 0.0;
                else
                    value = (s ? -1.0 : 1.0) * pow(2.0, -126.0) * (double)f / 128.0;
            }
            else
            {
                value = (s ? -1.0 : 1.0) * pow(2.0, double(e - 127)) * ((double)f / 128.0 + 1.0);
            }
            return value;
        }

        void ASCATReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 654)
                return;

            // float receiver_gain = packet.payload[104 + 0 * 2 + 0] << 8 | packet.payload[104 + 0 * 2 + 1];
            // receiver_gain = (receiver_gain / 65535.0) * 20;

            int channel = packet.header.apid - 208;

            if (6 > channel && channel >= 0) // Echo Readings
            {
                channels[channel].push_back(std::vector<float>(256));

                for (int i = 0; i < 256; i++)
                {
                    uint16_t sample = packet.payload[140 + i * 2 + 0] << 8 | packet.payload[140 + i * 2 + 1];

                    double value = parse_uint_to_float(sample);

                    channels[channel][lines[channel]][i] = value;
                    channels_img[channel][lines[channel] * 256 + i] = value / 100;
                }

                timestamps[channel].push_back(ccsds::crcCheckVerticalParity(packet) ? ccsds::parseCCSDSTimeFull(packet, 10957) : -1);

                // Frame counter
                lines[channel]++;

                channels_img[channel].resize((lines[channel] + 1) * 256);
            }

            channel -= 16;

            if (6 > channel && channel >= 0) // Noise Readings
            {
                noise_channels[channel].push_back(std::vector<float>(256));

                for (int i = 0; i < 256; i++)
                {
                    uint16_t sample = packet.payload[140 + i * 2 + 0] << 8 | packet.payload[140 + i * 2 + 1];

                    double value = parse_uint_to_float(sample);

                    noise_channels[channel][noise_lines[channel]][i] = value;
                }

                noise_timestamps[channel].push_back(ccsds::crcCheckVerticalParity(packet) ? ccsds::parseCCSDSTimeFull(packet, 10957) : -1);

                // Frame counter
                noise_lines[channel]++;
            }
        }

        image::Image ASCATReader::getChannelImg(int channel)
        {
            return image::Image(channels_img[channel].data(), 16, 256, lines[channel], 1);
        }

        std::vector<std::vector<float>> ASCATReader::getChannel(int channel)
        {
            return channels[channel];
        }
    } // namespace avhrr
} // namespace metop