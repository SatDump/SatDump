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
#include "common/ccsds/ccsds_1_0_proba/vcdu.h"
#include <fstream>

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::ifstream idk_file(argv[1]);

    uint8_t buffer1[1279];

    while (!idk_file.eof())
    {
        idk_file.read((char *)buffer1, 1279);

        auto vcdu = ccsds::ccsds_1_0_proba::parseVCDU(buffer1);

        logger->critical(vcdu.spacecraft_id);
    }
}
