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
#include "libs/miniz/miniz.h"
#include "common/image/image.h"

#include "common/ccsds/ccsds_standard/vcdu.h"
#include "common/ccsds/ccsds_standard/demuxer.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t cadu[1119];

    ccsds::ccsds_standard::Demuxer demuxer_vcid0(1103, false);

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)cadu, 1119);

        // Parse this transport frame
        ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 0)
        {
            //  data_out.write((char *)cadu, 1119);
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 2047)
                    continue;

                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 99)
                {
                    pkt.payload.resize(1000 - 6);
                    data_out.write((char *)pkt.header.raw, 6);
                    data_out.write((char *)pkt.payload.data(), 1000 - 6);
                }
            }
        }
    }
}