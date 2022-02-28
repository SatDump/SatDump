#include "modis_reader.h"

#include <map>
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace eos
{
    namespace modis
    {
        MODISReader::MODISReader()
        {
            for (int i = 0; i < 31; i++)
                channels1000m[i].resize(1354 * 10);
            for (int i = 0; i < 5; i++)
                channels500m[i].resize(1354 * 2 * 20);
            for (int i = 0; i < 2; i++)
                channels250m[i].resize(1354 * 4 * 40);
            lines = 0;
            day_count = 0;
            night_count = 0;
        }

        MODISReader::~MODISReader()
        {
            for (int i = 0; i < 31; i++)
                channels1000m[i].clear();
            for (int i = 0; i < 5; i++)
                channels500m[i].clear();
            for (int i = 0; i < 2; i++)
                channels250m[i].clear();
        }

        /*
        The official MODIS User Guide states this is an "exclusive-or checksum",
        but that did not lead to any useful results.

        Looking online, I was however able to find a working and proper
        implementation :
        https://oceancolor.gsfc.nasa.gov/docs/ocssw/check__checksum_8c_source.html

        In the code linked above, the proper specification is :
        -------------------------------------------------
        82                FYI, The checksum is computed as follows:
        83                1: Within each packet, the 12-bit science data (or test data)
        84                   is summed into a 16-bit register, with overflows ignored.
        85                2: After all the data is summed, the 16-bit result is right
        86                   shifted four bits, with zeros inserted into the leading
        87                   four bits.
        88                3: The 12 bits left in the lower end of the word becomes the
        89                   checksum.
        -------------------------------------------------

        The documentation is hence... Not so helpful with this :-)
         */
        uint16_t MODISReader::compute_crc(uint16_t *data, int size)
        {
            uint16_t crc = 0;
            for (int i = 0; i < size; i++)
                crc += data[i];
            crc >>= 4;
            return crc;
        }

        void MODISReader::processDayPacket(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            // Filter out calibration packets
            if (header.type_flag == 1 || header.earth_frame_data_count > 1354 /*|| header.mirror_side > 1*/)
                return;

            repackBytesTo12bits(&packet.payload[12], 624, modis_ifov);

            // Check CRC
            if (compute_crc(modis_ifov, 415) != modis_ifov[415])
                return;

            // std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

            int position = header.earth_frame_data_count - 1;

            if (position == 0 && packet.header.sequence_flag == 1 && lastScanCount != header.scan_count)
            {
                lines += 10;

                for (int i = 0; i < 31; i++)
                    channels1000m[i].resize((lines + 10) * 1354);
                for (int i = 0; i < 5; i++)
                    channels500m[i].resize((lines * 2 + 20) * 1354 * 2);
                for (int i = 0; i < 2; i++)
                    channels250m[i].resize((lines * 4 + 40) * 1354 * 4);

                double timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);
                for (int i = -5; i < 5; i++)
                    timestamps_1000.push_back(timestamp + i * 0.162); // 1000m timestamps
                for (int i = -10; i < 10; i++)
                    timestamps_500.push_back(timestamp + i * 0.081); // 500m timestamps
                for (int i = -20; i < 20; i++)
                    timestamps_250.push_back(timestamp + i * 0.0405); // 250m timestamps
            }

            lastScanCount = header.scan_count;

            if (packet.header.sequence_flag == 1) // Contains IFOVs 1-5
            {
                // Channel 1-2 (250m)
                for (int c = 0; c < 2; c++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        for (int y = 0; y < 4; y++)
                        {
                            channels250m[c][((lines + 9) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[0 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 8) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[83 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 7) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[166 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 6) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[249 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 5) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[332 + (c * 16) + (i * 4) + y] << 4;
                        }
                    }
                }

                // Channel 3-7 (500m)
                for (int c = 0; c < 5; c++)
                {
                    for (int i = 0; i < 2; i++)
                    {
                        for (int y = 0; y < 2; y++)
                        {
                            channels500m[c][((lines + 9) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[32 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 8) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[115 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 7) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[198 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 6) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[281 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 5) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[364 + (c * 4) + (i * 2) + y] << 4;
                        }
                    }
                }

                // 28 1000m channels
                for (int i = 0; i < 31; i++)
                {
                    channels1000m[i][(lines + 9) * 1354 + position] = modis_ifov[52 + i] << 4;
                    channels1000m[i][(lines + 8) * 1354 + position] = modis_ifov[135 + i] << 4;
                    channels1000m[i][(lines + 7) * 1354 + position] = modis_ifov[218 + i] << 4;
                    channels1000m[i][(lines + 6) * 1354 + position] = modis_ifov[301 + i] << 4;
                    channels1000m[i][(lines + 5) * 1354 + position] = modis_ifov[384 + i] << 4;
                }
            }
            else if (packet.header.sequence_flag == 2) // Contains IFOVs 6-10
            {
                // Channel 1-2 (250m)
                for (int c = 0; c < 2; c++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        for (int y = 0; y < 4; y++)
                        {
                            channels250m[c][((lines + 4) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[0 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 3) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[83 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 2) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[166 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 1) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[249 + (c * 16) + (i * 4) + y] << 4;
                            channels250m[c][((lines + 0) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[332 + (c * 16) + (i * 4) + y] << 4;
                        }
                    }
                }

                // Channel 3-7 (500m)
                for (int c = 0; c < 5; c++)
                {
                    for (int i = 0; i < 2; i++)
                    {
                        for (int y = 0; y < 2; y++)
                        {
                            channels500m[c][((lines + 4) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[32 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 3) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[115 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 2) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[198 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 1) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[281 + (c * 4) + (i * 2) + y] << 4;
                            channels500m[c][((lines + 0) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[364 + (c * 4) + (i * 2) + y] << 4;
                        }
                    }
                }

                // 28 1000m channels
                for (int i = 0; i < 31; i++)
                {
                    channels1000m[i][(lines + 4) * 1354 + position] = modis_ifov[52 + i] << 4;
                    channels1000m[i][(lines + 3) * 1354 + position] = modis_ifov[135 + i] << 4;
                    channels1000m[i][(lines + 2) * 1354 + position] = modis_ifov[218 + i] << 4;
                    channels1000m[i][(lines + 1) * 1354 + position] = modis_ifov[301 + i] << 4;
                    channels1000m[i][(lines + 0) * 1354 + position] = modis_ifov[384 + i] << 4;
                }
            }
        }

        void MODISReader::processNightPacket(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            // Filter out calibration packets
            if (header.type_flag == 1 || header.earth_frame_data_count > 1354 /*|| header.mirror_side > 1*/)
                return;

            repackBytesTo12bits(&packet.payload[12], 258, modis_ifov);

            // Check CRC
            if (compute_crc(modis_ifov, 258) != modis_ifov[258])
                return;

            // std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

            int position = header.earth_frame_data_count - 1;

            if (position == 0 && lastScanCount != header.scan_count)
            {
                lines += 10;

                for (int i = 0; i < 31; i++)
                    channels1000m[i].resize((lines + 10) * 1354);
                for (int i = 0; i < 5; i++)
                    channels500m[i].resize((lines * 2 + 20) * 1354 * 2);
                for (int i = 0; i < 2; i++)
                    channels250m[i].resize((lines * 4 + 40) * 1354 * 4);

                double timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);
                for (int i = -5; i < 5; i++)
                    timestamps_1000.push_back(timestamp + i * 0.162); // 1000m timestamps
                for (int i = -10; i < 10; i++)
                    timestamps_500.push_back(timestamp + i * 0.081); // 500m timestamps
                for (int i = -20; i < 20; i++)
                    timestamps_250.push_back(timestamp + i * 0.0405); // 250m timestamps
            }

            lastScanCount = header.scan_count;

            // 28 1000m channels
            for (int i = 0; i < 16; i++)
            {
                channels1000m[14 + i][(lines + 9) * 1354 + position] = modis_ifov[0 + i] << 4;
                channels1000m[14 + i][(lines + 8) * 1354 + position] = modis_ifov[17 + i] << 4;
                channels1000m[14 + i][(lines + 7) * 1354 + position] = modis_ifov[34 + i] << 4;
                channels1000m[14 + i][(lines + 6) * 1354 + position] = modis_ifov[51 + i] << 4;
                channels1000m[14 + i][(lines + 5) * 1354 + position] = modis_ifov[68 + i] << 4;
                channels1000m[14 + i][(lines + 4) * 1354 + position] = modis_ifov[85 + i] << 4;
                channels1000m[14 + i][(lines + 3) * 1354 + position] = modis_ifov[102 + i] << 4;
                channels1000m[14 + i][(lines + 2) * 1354 + position] = modis_ifov[119 + i] << 4;
                channels1000m[14 + i][(lines + 1) * 1354 + position] = modis_ifov[136 + i] << 4;
                channels1000m[14 + i][(lines + 0) * 1354 + position] = modis_ifov[153 + i] << 4;
            }
        }

        void MODISReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 10)
                return;

            MODISHeader modisHeader(packet);

            if (modisHeader.packet_type == MODISHeader::DAY_GROUP)
            {
                if (packet.payload.size() < 636)
                    return;

                day_count++;
                processDayPacket(packet, modisHeader);
            }
            else if (modisHeader.packet_type == MODISHeader::NIGHT_GROUP)
            {
                if (packet.payload.size() < 270)
                    return;

                night_count++;
                processNightPacket(packet, modisHeader);
            }
        }

        image::Image<uint16_t> MODISReader::getImage250m(int channel)
        {
            return image::Image<uint16_t>(channels250m[channel].data(), 1354 * 4, lines * 4, 1);
        }

        image::Image<uint16_t> MODISReader::getImage500m(int channel)
        {
            return image::Image<uint16_t>(channels500m[channel].data(), 1354 * 2, lines * 2, 1);
        }

        image::Image<uint16_t> MODISReader::getImage1000m(int channel)
        {
            return image::Image<uint16_t>(channels1000m[channel].data(), 1354, lines, 1);
        }
    } // namespace modis
} // namespace eos