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
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/mpdu.h"
#include <fstream>

#include "common/codings/randomization.h"

#include "common/image/image.h"
#include "common/repack.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::ifstream idk_file(argv[1]);
    std::ofstream idk_file2(argv[2]);

    uint8_t buffer1[1024];


    while (!idk_file.eof())
    {
        idk_file.read((char *)buffer1, 1024);

        derand_ccsds(&buffer1[4], 1024 - 4);

        idk_file2.write((char *)&buffer1[4], 892);
    }
}