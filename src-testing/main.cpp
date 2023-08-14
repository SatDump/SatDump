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

#include "common/ccsds/ccsds_weather/vcdu.h"
#include "common/ccsds/ccsds_weather/demuxer.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t cadu[1024];

    ccsds::ccsds_weather::Demuxer demuxer_vcid1(884, true, 0);

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1024);

        // Parse this transport frame
        ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

        //  printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 2)
        {
            // data_out.write((char *)cadu, 1024);

            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {

                printf("APID %d\n", pkt.header.apid);
                if (pkt.header.apid == 810)
                {
                    // atms_reader.work(pkt);
                    printf("LEN %d\n", pkt.header.packet_length);
                    pkt.payload.resize(10240 - 6);
                    data_out.write((char *)pkt.header.raw, 6);
                    data_out.write((char *)pkt.payload.data(), 10240 - 6);
                }
            }
        }
    }
}