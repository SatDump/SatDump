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
#include <vector>
//#include "common/image/image.h"
#include "common/repack.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include <iostream>

int main(int argc, char *argv[])
{
    initLogger();

    uint8_t cadu[256];
    std::ifstream cadu_file(argv[1]);

    ccsds::ccsds_1_0_1024::Demuxer demuxer(208);

    std::ofstream output_file("test.ccsds");

    while (!cadu_file.eof())
    {
        cadu_file.read((char *)cadu, 256);
        ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

        // logger->critical(vcdu.vcid);

        if (vcdu.vcid == 0)
        {
            auto ccsds_frames = demuxer.work(cadu);

            for (ccsds::CCSDSPacket pkt : ccsds_frames)
            {
                if (pkt.header.apid == 2047)
                    continue;

                std::cout << "APID " << pkt.header.apid << std::endl;

                if (pkt.header.apid == 905)
                {
                    std::cout << "SIZE " << pkt.payload.size() << std::endl;
                    output_file.write((char *)pkt.header.raw, 6);
                    output_file.write((char *)pkt.payload.data(), 76);
                }
            }
        }
    }
}
