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

#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/simple_deframer.h"
#include <cstdint>
#include <cstdio>
#include <fstream>

#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"

#include <cstring>

#include "common/repack.h"

#include "image/bayer/bayer.h"

#include "common/codings/reedsolomon/reedsolomon.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);

    uint8_t cadu[259];

    ccsds::ccsds_tm::Demuxer vcid_demuxer(105, false);

    std::ofstream test_t("/tmp/test.bin");

    reedsolomon::ReedSolomon rs_check(reedsolomon::RS223);

    std::vector<uint16_t> msi_channel;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)&cadu, 259);

        // Parse this transport frame
        ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

        //  printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 1)
        {
            auto pkts = vcid_demuxer.work(cadu);

            for (auto pkt : pkts)
            {
                //  printf("APID %d \n", pkt.header.apid);
                if (pkt.header.apid == 300)
                {
                    // 1027 ?
                    // 1028 ?
                    // 1031 !!!!!
                    // 1032 !!!!!

                    // printf("APID %d SIZE %d\n", pkt.header.apid, pkt.payload.size());

                    // printf("APID %d SIZE %d PKTSIZE %d\n", pkt.header.apid, pkt.payload.size(), pkt.header.packet_length);

                    uint16_t current = pkt.payload[0] << 8 | pkt.payload[1];
                    uint16_t volage = pkt.payload[2] << 8 | pkt.payload[3];

                    //                    printf("------------- %d A %f V \n", current, volage / 1000.0);

                    // pkt.payload.resize(150);

                    // test_t.write((char *)pkt.header.raw, 6);
                    // test_t.write((char *)pkt.payload.data(), pkt.payload.size() - 6);
                }
                else if (pkt.header.apid == 500)
                {
                    printf("APID %d SIZE %d PKTSIZE %d\n", pkt.header.apid, pkt.payload.size(), pkt.header.packet_length);

                    test_t.write((char *)pkt.payload.data(), pkt.payload.size());
                }
            }
        }
    }

    image::Image img(msi_channel.data(), 16, 218, msi_channel.size() / 218, 1);
    image::save_png(img, "test_earthcare.png");
}
