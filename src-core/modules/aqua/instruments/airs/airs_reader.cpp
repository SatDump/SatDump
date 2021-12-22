#include "airs_reader.h"
#include "utils.h"
#include "common/ccsds/ccsds_time.h"

namespace aqua
{
    namespace airs
    {
        AIRSReader::AIRSReader()
        {
            for (int i = 0; i < 2666; i++)
            {
                channels[i] = new unsigned short[10000 * 90];
            }
            for (int i = 0; i < 4; i++)
            {
                hd_channels[i] = new unsigned short[10000 * 90 * 9];
            }
            lines = 0;
            timestamps_ifov.push_back(std::vector<double>(90));
            std::fill(timestamps_ifov[lines].begin(), timestamps_ifov[lines].end(), -1);
        }

        AIRSReader::~AIRSReader()
        {
            for (int i = 0; i < 2666; i++)
            {
                delete[] channels[i];
            }
            for (int i = 0; i < 4; i++)
            {
                delete[] hd_channels[i];
            }
        }

        void AIRSReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 4280)
                return;

            uint16_t counter = packet.payload[10] << 8 | packet.payload[11];
            uint16_t pix_pos = counter;

            if (counter < 278)
                pix_pos -= 22;
            else if (counter < 534)
                pix_pos -= 278;
            else
                pix_pos -= 534;

            // Decode 14-bits channels
            {
                int pos = 12;
                std::vector<uint16_t> values = bytesTo14bits(&packet.payload.data()[pos], 0, 903);
                // First 3 channels
                for (int channel = 0; channel < 514; channel++)
                    channels[channel][lines * 90 + pix_pos] = values[channel] << 2;
            }

            // Decode 13-bits channels
            {
                int pos = 911;
                std::vector<uint16_t> values = bytesTo13bits(&packet.payload.data()[pos], 5, 4003);
                // First 3 channels
                for (int channel = 514; channel < 1611; channel++)
                    channels[channel][lines * 90 + pix_pos] = values[channel - 514] << 3;
            }

            std::vector<uint16_t> values_hd;

            // Decode 12-bits channels
            {
                int pos = 2692;
                std::vector<uint16_t> values = bytesTo12bits(&packet.payload.data()[pos], 5 + 12, 4003);
                // First 3 channels
                for (int channel = 1611; channel < 2666; channel++)
                    channels[channel][lines * 90 + pix_pos] = values[channel - 1611] << 3;

                values_hd.insert(values_hd.end(), &values[1055 - 288], &values[1055]);
            }

            // Recompose HD channels
            for (int channel = 0; channel < 4; channel++)
            {
                for (int i = 0; i < 8; i++)
                {
                    for (int y = 0; y < 9; y++)
                    {
                        hd_channels[channel][(lines * 9 + (8 - y)) * (90 * 8) + (pix_pos * 8) + (i)] = values_hd[/*(y * 8 + i)*/ (i * 9 + y) * 4 + channel] << 3;
                    }
                }
            }

            // Timestamp
            double timestamp = ccsds::parseCCSDSTimeFullRawUnsegmented(&packet.payload[1], -4383, 15.3e-6);
            timestamps_ifov[lines][pix_pos] = timestamp;

            // Frame counter
            if (counter == 22 || counter == 278 || counter == 534)
            {
                lines++;
                timestamps_ifov.push_back(std::vector<double>(90));
                std::fill(timestamps_ifov[lines].begin(), timestamps_ifov[lines].end(), -1);
            }
        }

        image::Image<uint16_t> AIRSReader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 90, lines, 1);
            img.normalize();
            img.equalize();
            img.mirror(true, false);
            return img;
        }

        image::Image<uint16_t> AIRSReader::getHDChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(hd_channels[channel], 90 * 8, lines * 9, 1);
            img.normalize();
            img.equalize();
            img.mirror(true, false);
            return img;
        }
    } // namespace airs
} // namespace aqua