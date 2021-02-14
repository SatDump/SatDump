#include "modis_reader.h"

#include <iostream>
#include <map>

#define DAY_GROUP 0b000
#define NIGHT_GROUP 0b001

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

std::vector<uint16_t> bytesTo12bits(std::vector<uint8_t> &in, int offset, int skip, int lengthToConvert)
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

void MODISReader::processDayPacket(libccsds::CCSDSPacket &packet, MODISHeader &header)
{
    // Filter out calibration packets
    if (header.type_flag == 1 || header.earth_frame_data_count > 1354 || header.mirror_side > 1)
        return;

    //std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

    int position = header.earth_frame_data_count - 1;

    if (position == 0 && packet.header.sequence_flag == 1 && lastScanCount != header.scan_count)
    {
        lines += 10;
    }

    lastScanCount = header.scan_count;

    if (packet.header.sequence_flag == 1)
    {
        // Contains IFOVs 1-5
        std::vector<uint16_t> values = bytesTo12bits(packet.payload, 12, 1, 500);

        // Channel 1-2 (250m)
        for (int c = 0; c < 2; c++)
        {
            for (int i = 0; i < 4; i++)
            {
                for (int y = 0; y < 4; y++)
                {
                    channels250m[c][((lines + 9) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[0 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 8) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[83 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 7) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[166 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 6) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[249 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 5) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[332 + (c * 16) + (i * 4) + y] * 15;
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
                    channels500m[c][((lines + 9) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[32 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 8) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[115 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 7) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[198 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 6) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[281 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 5) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[364 + (c * 4) + (i * 2) + y] * 15;
                }
            }
        }

        // 28 1000m channels
        for (int i = 0; i < 31; i++)
        {
            channels1000m[i][(lines + 9) * 1354 + position] = values[52 + i] * 15;
            channels1000m[i][(lines + 8) * 1354 + position] = values[135 + i] * 15;
            channels1000m[i][(lines + 7) * 1354 + position] = values[218 + i] * 15;
            channels1000m[i][(lines + 6) * 1354 + position] = values[301 + i] * 15;
            channels1000m[i][(lines + 5) * 1354 + position] = values[384 + i] * 15;
        }
    }
    else if (packet.header.sequence_flag == 2)
    {
        // Contains IFOVs 6-10
        std::vector<uint16_t> values = bytesTo12bits(packet.payload, 12, 1, 500);

        // Channel 1-2 (250m)
        for (int c = 0; c < 2; c++)
        {
            for (int i = 0; i < 4; i++)
            {
                for (int y = 0; y < 4; y++)
                {
                    channels250m[c][((lines + 4) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[0 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 3) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[83 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 2) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[166 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 1) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[249 + (c * 16) + (i * 4) + y] * 15;
                    channels250m[c][((lines + 0) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = values[332 + (c * 16) + (i * 4) + y] * 15;
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
                    channels500m[c][((lines + 4) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[32 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 3) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[115 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 2) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[198 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 1) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[281 + (c * 4) + (i * 2) + y] * 15;
                    channels500m[c][((lines + 0) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = values[364 + (c * 4) + (i * 2) + y] * 15;
                }
            }
        }

        // 28 1000m channels
        for (int i = 0; i < 31; i++)
        {
            channels1000m[i][(lines + 4) * 1354 + position] = values[52 + i] * 15;
            channels1000m[i][(lines + 3) * 1354 + position] = values[135 + i] * 15;
            channels1000m[i][(lines + 2) * 1354 + position] = values[218 + i] * 15;
            channels1000m[i][(lines + 1) * 1354 + position] = values[301 + i] * 15;
            channels1000m[i][(lines + 0) * 1354 + position] = values[384 + i] * 15;
        }
    }
}

void MODISReader::processNightPacket(libccsds::CCSDSPacket &packet, MODISHeader &header)
{
    // Filter out calibration packets
    if (header.type_flag == 1 || header.earth_frame_data_count > 1354 || header.mirror_side > 1)
        return;

    int position = header.earth_frame_data_count;

    if (position == 0 && packet.header.sequence_flag == 1 && lastScanCount != header.scan_count)
    {
        lines += 10;
    }

    lastScanCount = header.scan_count;

    std::vector<uint16_t> values = bytesTo12bits(packet.payload, 12, 1, 200);

    // 28 1000m channels
    for (int i = 0; i < 16; i++)
    {
        channels1000m[14 + i][(lines + 9) * 1354 + position] = values[0 + i] * 15;
        channels1000m[14 + i][(lines + 8) * 1354 + position] = values[17 + i] * 15;
        channels1000m[14 + i][(lines + 7) * 1354 + position] = values[34 + i] * 15;
        channels1000m[14 + i][(lines + 6) * 1354 + position] = values[51 + i] * 15;
        channels1000m[14 + i][(lines + 5) * 1354 + position] = values[68 + i] * 15;
        channels1000m[14 + i][(lines + 4) * 1354 + position] = values[85 + i] * 15;
        channels1000m[14 + i][(lines + 3) * 1354 + position] = values[102 + i] * 15;
        channels1000m[14 + i][(lines + 2) * 1354 + position] = values[119 + i] * 15;
        channels1000m[14 + i][(lines + 1) * 1354 + position] = values[136 + i] * 15;
        channels1000m[14 + i][(lines + 0) * 1354 + position] = values[153 + i] * 15;
    }
}

int i = 0;

void MODISReader::work(libccsds::CCSDSPacket &packet)
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

    if (modisHeader.packet_type == DAY_GROUP)
    {
        if (packet.payload.size() < 622)
            return;

        day_count++;
        processDayPacket(packet, modisHeader);
    }
    else if (modisHeader.packet_type == NIGHT_GROUP)
    {
        if (packet.payload.size() < 256)
            return;

        night_count++;
        processNightPacket(packet, modisHeader);
    }
}

cimg_library::CImg<unsigned short> MODISReader::getImage250m(int channel)
{
    return cimg_library::CImg<unsigned short>(channels250m[channel], 1354 * 4, lines * 4);
}

cimg_library::CImg<unsigned short> MODISReader::getImage500m(int channel)
{
    return cimg_library::CImg<unsigned short>(channels500m[channel], 1354 * 2, lines * 2);
}

cimg_library::CImg<unsigned short> MODISReader::getImage1000m(int channel)
{
    return cimg_library::CImg<unsigned short>(channels1000m[channel], 1354, lines);
}