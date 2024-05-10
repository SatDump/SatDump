#include "airs_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace aqua
{
    namespace airs
    {
        AIRSReader::AIRSReader()
        {
            for (int i = 0; i < 2666; i++)
                channels[i].resize(90);

            for (int i = 0; i < 4; i++)
                hd_channels[i].resize(90 * 9 * 8);

            lines = 0;
            timestamps_ifov.push_back(std::vector<double>(90, -1));
        }

        AIRSReader::~AIRSReader()
        {
            for (int i = 0; i < 2666; i++)
                channels[i].clear();

            for (int i = 0; i < 4; i++)
                hd_channels[i].clear();
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

            if (pix_pos > 89)
                return;

            // Decode 14-bits channels
            {
                repackBytesTo14bits(&packet.payload[12], 1581, line_buffer);
                for (int channel = 0; channel < 514; channel++)
                    channels[channel][lines * 90 + 89 - pix_pos] = line_buffer[channel] << 2;
            }

            // Decode 13-bits channels
            {
                shift_array_left(&packet.payload[911], 3369 - 1, 4, shifted_buffer);
                repackBytesTo13bits(shifted_buffer, 3369, line_buffer);
                for (int channel = 514; channel < 1611; channel++)
                    channels[channel][lines * 90 + 89 - pix_pos] = line_buffer[channel - 514] << 3;
            }

            std::vector<uint16_t> values_hd;

            // Decode 12-bits channels
            {
                shift_array_left(&packet.payload[2693], 1587 - 1, 7, shifted_buffer);
                repackBytesTo12bits(shifted_buffer, 1587, line_buffer);
                for (int channel = 1611; channel < 2666; channel++)
                    channels[channel][lines * 90 + 89 - pix_pos] = line_buffer[channel - 1611] << 4;
                values_hd.insert(values_hd.end(), &line_buffer[1055 - 288], &line_buffer[1055]);
            }

            // Recompose HD channels
            for (int channel = 0; channel < 4; channel++)
                for (int i = 0; i < 8; i++)
                    for (int y = 0; y < 9; y++)
                        hd_channels[channel][(lines * 9 + (8 - y)) * (90 * 8) + (90 * 8 - 1) - ((pix_pos * 8) + i)] = values_hd[/*(y * 8 + i)*/ (i * 9 + y) * 4 + channel] << 4;

            // Timestamp
            timestamps_ifov[lines][pix_pos] = ccsds::parseCCSDSTimeFullRawUnsegmented(&packet.payload[1], -4383, 15.3e-6);

            // Frame counter
            if (counter == 22 || counter == 278 || counter == 534)
            {
                lines++;
                timestamps_ifov.push_back(std::vector<double>(90, -1));

                for (int i = 0; i < 2666; i++)
                    channels[i].resize((lines + 1) * 90);
                for (int i = 0; i < 4; i++)
                    hd_channels[i].resize(((lines + 1) * 9) * 90 * 8);
            }
        }

        image::Image AIRSReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 90, lines, 1);
        }

        image::Image AIRSReader::getHDChannel(int channel)
        {
            return image::Image(hd_channels[channel].data(), 16, 90 * 8, lines * 9, 1);
        }
    } // namespace airs
} // namespace aqua