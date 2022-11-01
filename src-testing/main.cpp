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
#include <fstream>

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::ifstream idk_file(argv[1]);

    std::ofstream idk_file2(argv[2]);

    uint8_t buffer1[1024];

    ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid1(884, true, 0);

    while (!idk_file.eof())
    {
        idk_file.read((char *)buffer1, 1024);

        auto vcdu = ccsds::ccsds_1_0_1024::parseVCDU(buffer1);

        // logger->critical(vcdu.vcid);

        if (vcdu.vcid == 4)
        {

            std::vector<ccsds::CCSDSPacket> pkts = demuxer_vcid1.work(buffer1);

            for (auto pkt : pkts)
            {
                logger->critical(pkt.header.apid);

                pkt.payload.resize(1000 - 6);
                idk_file2.write((char *)pkt.header.raw, 6);
                idk_file2.write((char *)pkt.payload.data(), 1000 - 6);
            }
        }
    }
}
