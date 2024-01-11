#pragma once

#include "cutils.h"

struct ELEKTROImage
{
    std::string link;
    time_t timestamp;
};

std::vector<ELEKTROImage> get_list_of_current_day_elektro(time_t current_moscow_time, int satnum);
std::vector<ccsds::CCSDSPacket> encode_elektro_dump(std::string path, time_t timestamp, int apid);