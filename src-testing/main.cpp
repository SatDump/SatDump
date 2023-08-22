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
#include "common/codings/rotation.h"

int main(int argc, char *argv[])
{
    initLogger();

    int8_t *buffer = new int8_t[8192];

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    while (!data_in.eof())
    {
        data_in.read((char *)buffer, 8192);

        rotate_soft(buffer, 8192, PHASE_90, false);

        data_out.write((char *)buffer, 8192);
    }
}