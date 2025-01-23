/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include <fstream>

int main(int argc, char *argv[])
{
    initLogger();

    uint8_t pkt[224];

    std::ifstream data_in("/home/alan/Downloads/SatDump/build/live_output/2025-01-23_15-24_elektro_ggak_1.693 GHz/elektro_ggak.cadu");
    std::ofstream data_ou("/home/alan/Downloads/SatDump/build/live_output/2025-01-23_15-24_elektro_ggak_1.693 GHz/elektro_ggak_p.cadu");

    while (!data_in.eof())
    {
        data_in.read((char *)pkt, 224);
        data_in.read((char *)pkt, 224);

        int marker = pkt[4] >> 4;

        if (marker == 0)
            continue;
        else if (marker == 2)
            continue;
        else if (marker == 1)
            continue;
        else if (marker == 4)
            continue;
        else if (marker == 5)
            continue;
        else if (marker == 7)
            continue;
        else if (marker == 3)
            data_ou.write((char *)pkt, 224);
        else if (marker == 6)
            continue;

        logger->trace(marker);
    }
}
