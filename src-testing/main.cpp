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
#include <cstring>
#include "common/image/image.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();
    completeLoggerInit();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ifstream data_in2(argv[2], std::ios::binary);
    std::ofstream data_out(argv[3], std::ios::binary);

    uint8_t derand[314];

    data_in2.read((char *)derand, 314);

    while (!data_in.eof())
    {
        uint8_t frm[314];
        data_in.read((char *)frm, 314);

        for (int i = 0; i < 314; i++)
            frm[i] ^= derand[i];

        data_out.write((char *)frm, 314);
    }
}