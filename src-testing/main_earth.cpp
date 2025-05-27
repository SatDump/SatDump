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

#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include <fstream>
#include "common/simple_deframer.h"

#include "image/image.h"
#include "image/processing.h"
#include "image/io.h"

#include <cstring>

#include "common/repack.h"

#include "image/bayer/bayer.h"

#include "common/codings/reedsolomon/reedsolomon.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);

    uint8_t cadu[1279];

    ccsds::ccsds_tm::Demuxer vcid_demuxer(1109, false);

    std::ofstream test_t("/tmp/test.bin");

    reedsolomon::ReedSolomon rs_check(reedsolomon::RS223);

    std::vector<uint16_t> msi_channel;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)&cadu, 1279);

        // Check RS
        int errors[5];
        rs_check.decode_interlaved(cadu, true, 5, errors);
        if (errors[0] < 0 || errors[1] < 0 || errors[2] < 0 || errors[3] < 0 || errors[4] < 0)
            continue;

        // Parse this transport frame
        ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

        // printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 6)
        {
            auto pkts = vcid_demuxer.work(cadu);

            for (auto pkt : pkts)
            {
                // printf("APID %d \n", pkt.header.apid);
                if (pkt.header.apid == 1228)
                {
                    // 1027 ?
                    // 1028 ?
                    // 1031 !!!!!
                    // 1032 !!!!!

                    printf("APID %d SIZE %d\n", pkt.header.apid, pkt.payload.size());

                    pkt.payload.resize(3000);

                    int id = pkt.payload[23];

                    if (id == 0)
                    {
                        test_t.write((char *)pkt.header.raw, 6);
                        test_t.write((char *)pkt.payload.data(), pkt.payload.size());

                        for (int i = 0; i < 218; i++)
                        {
                            uint16_t val = pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1];
                            msi_channel.push_back(val);
                        }
                    }
                }
            }
        }
    }

    image::Image img(msi_channel.data(), 16, 218, msi_channel.size() / 218, 1);
    image::save_png(img, "test_earthcare.png");
}
