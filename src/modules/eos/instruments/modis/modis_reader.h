#pragma once

#include <ccsds/ccsds.h>
#include <cmath>
#include <map>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

struct MODISHeader
{
    MODISHeader(libccsds::CCSDSPacket &pkt)
    {
        day_count = pkt.payload[0] << 8 | pkt.payload[1];
        coarse_time = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
        fine_time = pkt.payload[6] << 8 | pkt.payload[7];

        quick_look = pkt.payload[8] >> 7;
        packet_type = (pkt.payload[8] >> 4) % (int)pow(2, 3);
        scan_count = (pkt.payload[8] >> 1) % (int)pow(2, 3);
        mirror_side = pkt.payload[8] % 2;

        type_flag = pkt.payload[9] >> 7;
        earth_frame_data_count = (pkt.payload[9] % (int)pow(2, 7)) << 4 | pkt.payload[10] >> 4;
    }

    uint16_t day_count;
    uint32_t coarse_time;
    uint16_t fine_time;

    bool quick_look;
    uint8_t packet_type;
    uint8_t scan_count;
    bool mirror_side;

    bool type_flag;
    uint16_t earth_frame_data_count;
};

class MODISReader
{
private:
    int lastScanCount;
    unsigned short *channels1000m[31];
    unsigned short *channels500m[5];
    unsigned short *channels250m[2];
    void processDayPacket(libccsds::CCSDSPacket &packet, MODISHeader &header);
    void processNightPacket(libccsds::CCSDSPacket &packet, MODISHeader &header);

public:
    MODISReader();
    int day_count, night_count, lines;
    void work(libccsds::CCSDSPacket &packet);
    cimg_library::CImg<unsigned short> getImage250m(int channel);
    cimg_library::CImg<unsigned short> getImage500m(int channel);
    cimg_library::CImg<unsigned short> getImage1000m(int channel);

    uint16_t common_day;
    uint32_t common_coarse;
};