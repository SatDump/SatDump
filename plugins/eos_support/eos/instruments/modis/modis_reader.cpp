#include "modis_reader.h"

#include <map>
#include "common/ccsds/ccsds_time.h"

namespace eos
{
    namespace modis
    {
        MODISReader::MODISReader()
        {
            for (int i = 0; i < 31; i++)
                channels1000m[i] = new unsigned short[10000 * 1354];
            for (int i = 0; i < 5; i++)
                channels500m[i] = new unsigned short[100000 * 1354 * 2];
            for (int i = 0; i < 2; i++)
                channels250m[i] = new unsigned short[100000 * 1354 * 4];
            lines = 0;
            day_count = 0;
            night_count = 0;
        }

        MODISReader::~MODISReader()
        {
            for (int i = 0; i < 31; i++)
                delete[] channels1000m[i];
            for (int i = 0; i < 5; i++)
                delete[] channels500m[i];
            for (int i = 0; i < 2; i++)
                delete[] channels250m[i];
        }

        std::vector<uint16_t> bytesTo12bits(std::vector<uint8_t> &in, int offset, int lengthToConvert)
        {
            std::vector<uint16_t> result;
            int pos = offset;
            for (int i = 0; i < lengthToConvert; i += 2)
            {
                uint16_t one = in[pos + 0] << 4 | in[pos + 1] >> 4;
                uint16_t two = (in[pos + 1] % (int)pow(2, 4)) << 8 | in[pos + 2];

                result.push_back(one);
                result.push_back(two);
                pos += 3;
            }

            return result;
        }

        void MODISReader::processDayPacket(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            // Filter out calibration packets
            if (header.type_flag == 1 || header.earth_frame_data_count > 1354 /*|| header.mirror_side > 1*/)
                return;

            //std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

            int position = header.earth_frame_data_count - 1;

            if (position == 0 && packet.header.sequence_flag == 1 && lastScanCount != header.scan_count)
            {
                lines += 10;

                double timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);
                for (int i = -5; i < 5; i++)
                    timestamps_1000.push_back(timestamp + i * 0.162); // 1000m timestamps
                for (int i = -10; i < 10; i++)
                    timestamps_500.push_back(timestamp + i * 0.081); // 500m timestamps
                for (int i = -20; i < 20; i++)
                    timestamps_250.push_back(timestamp + i * 0.0405); // 250m timestamps
            }

            lastScanCount = header.scan_count;

            if (packet.header.sequence_flag == 1)
            {
                // Contains IFOVs 1-5
                std::vector<uint16_t> values = bytesTo12bits(packet.payload, 12, 542);

                // Channel 1-2 (250m)
                for (int c = 0; c < 2; c++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        for (int y = 0; y < 4; y++)
                        {
                            channels250m[c][((lines + 9) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[0 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 8) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[83 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 7) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[166 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 6) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[249 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 5) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[332 + (c * 16) + (i * 4) + y] * 16;
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
                            channels500m[c][((lines + 9) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[32 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 8) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[115 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 7) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[198 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 6) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[281 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 5) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[364 + (c * 4) + (i * 2) + y] * 16;
                        }
                    }
                }

                // 28 1000m channels
                for (int i = 0; i < 31; i++)
                {
                    channels1000m[i][(lines + 9) * 1354 + position] = values[52 + i] * 16;
                    channels1000m[i][(lines + 8) * 1354 + position] = values[135 + i] * 16;
                    channels1000m[i][(lines + 7) * 1354 + position] = values[218 + i] * 16;
                    channels1000m[i][(lines + 6) * 1354 + position] = values[301 + i] * 16;
                    channels1000m[i][(lines + 5) * 1354 + position] = values[384 + i] * 16;
                }
            }
            else if (packet.header.sequence_flag == 2)
            {
                // Contains IFOVs 6-10
                std::vector<uint16_t> values = bytesTo12bits(packet.payload, 12, 542);

                // Channel 1-2 (250m)
                for (int c = 0; c < 2; c++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        for (int y = 0; y < 4; y++)
                        {
                            channels250m[c][((lines + 4) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[0 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 3) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[83 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 2) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[166 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 1) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[249 + (c * 16) + (i * 4) + y] * 16;
                            channels250m[c][((lines + 0) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[332 + (c * 16) + (i * 4) + y] * 16;
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
                            channels500m[c][((lines + 4) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[32 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 3) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[115 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 2) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[198 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 1) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[281 + (c * 4) + (i * 2) + y] * 16;
                            channels500m[c][((lines + 0) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[364 + (c * 4) + (i * 2) + y] * 16;
                        }
                    }
                }

                // 28 1000m channels
                for (int i = 0; i < 31; i++)
                {
                    channels1000m[i][(lines + 4) * 1354 + position] = values[52 + i] * 16;
                    channels1000m[i][(lines + 3) * 1354 + position] = values[135 + i] * 16;
                    channels1000m[i][(lines + 2) * 1354 + position] = values[218 + i] * 16;
                    channels1000m[i][(lines + 1) * 1354 + position] = values[301 + i] * 16;
                    channels1000m[i][(lines + 0) * 1354 + position] = values[384 + i] * 16;
                }
            }
        }

        void MODISReader::processNightPacket(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            // Filter out calibration packets
            if (header.type_flag == 1 || header.earth_frame_data_count > 1354 /*|| header.mirror_side > 1*/)
                return;

            //std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

            int position = header.earth_frame_data_count - 1;

            if (position == 0 && lastScanCount != header.scan_count)
            {
                lines += 10;

                double timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);
                for (int i = -5; i < 5; i++)
                    timestamps_1000.push_back(timestamp + i * 0.162); // 1000m timestamps
                for (int i = -10; i < 10; i++)
                    timestamps_500.push_back(timestamp + i * 0.081); // 500m timestamps
                for (int i = -20; i < 20; i++)
                    timestamps_250.push_back(timestamp + i * 0.0405); // 250m timestamps
            }

            lastScanCount = header.scan_count;

            std::vector<uint16_t> values = bytesTo12bits(packet.payload, 12, 254);

            // 28 1000m channels
            for (int i = 0; i < 16; i++)
            {
                channels1000m[14 + i][(lines + 9) * 1354 + position] = values[0 + i] * 16;
                channels1000m[14 + i][(lines + 8) * 1354 + position] = values[17 + i] * 16;
                channels1000m[14 + i][(lines + 7) * 1354 + position] = values[34 + i] * 16;
                channels1000m[14 + i][(lines + 6) * 1354 + position] = values[51 + i] * 16;
                channels1000m[14 + i][(lines + 5) * 1354 + position] = values[68 + i] * 16;
                channels1000m[14 + i][(lines + 4) * 1354 + position] = values[85 + i] * 16;
                channels1000m[14 + i][(lines + 3) * 1354 + position] = values[102 + i] * 16;
                channels1000m[14 + i][(lines + 2) * 1354 + position] = values[119 + i] * 16;
                channels1000m[14 + i][(lines + 1) * 1354 + position] = values[136 + i] * 16;
                channels1000m[14 + i][(lines + 0) * 1354 + position] = values[153 + i] * 16;
            }
        }

        void MODISReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 10)
                return;

            MODISHeader modisHeader(packet);

            // Check day validity
            if (modisHeader.day_count != common_day)
                return;

            // Check ms validity, allowing a 10% margin
            if (modisHeader.coarse_time > common_coarse + (common_coarse / 10) || modisHeader.coarse_time < common_coarse - (common_coarse / 10))
                return;

            if (modisHeader.packet_type == MODISHeader::DAY_GROUP)
            {
                if (packet.payload.size() < 622)
                    return;

                day_count++;
                processDayPacket(packet, modisHeader);
            }
            else if (modisHeader.packet_type == MODISHeader::NIGHT_GROUP)
            {
                if (packet.payload.size() < 256)
                    return;

                night_count++;
                processNightPacket(packet, modisHeader);
            }
        }

        image::Image<uint16_t> MODISReader::getImage250m(int channel)
        {
            return image::Image<uint16_t>(channels250m[channel], 1354 * 4, lines * 4, 1);
        }

        image::Image<uint16_t> MODISReader::getImage500m(int channel)
        {
            return image::Image<uint16_t>(channels500m[channel], 1354 * 2, lines * 2, 1);
        }

        image::Image<uint16_t> MODISReader::getImage1000m(int channel)
        {
            return image::Image<uint16_t>(channels1000m[channel], 1354, lines, 1);
        }
    } // namespace modis
} // namespace eos