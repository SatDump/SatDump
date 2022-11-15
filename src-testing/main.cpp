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

#if 0
    std::ifstream idk_file(argv[1]);
    std::ofstream idk_file2(argv[2]);

    uint8_t buffer1[1024];


    while (!idk_file.eof())
    {
        idk_file.read((char *)buffer1, 1024);

        derand_ccsds(&buffer1[4], 1024 - 4);

        idk_file2.write((char *)&buffer1[4], 892);
    }
#else
    std::ifstream idk_file(argv[1]);
    std::ofstream idk_file2(argv[2]);

    uint8_t buffer1[1034];

    ccsds::ccsds_1_0_1024::Demuxer demux_idk(1022, true, 0);

    image::Image<uint16_t> img_out1(1280, 3000, 1);
    image::Image<uint16_t> img_out2(1280, 3000, 1);
    image::Image<uint16_t> img_out3(1280, 3000, 1);
    uint16_t tirs_line[3898];
    int lines = 0;

    while (!idk_file.eof())
    {
        idk_file.read((char *)buffer1, 1034);

        auto vcdu = ccsds::ccsds_1_0_1024::parseVCDU(buffer1);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 5)
        {
            // idk_file2.write((char *)buffer1, 1034);

            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demux_idk.work(buffer1);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 1793)
                {
                    printf("SIZE %d\n", pkt.payload.size());

                    pkt.payload.resize(5848);
                    idk_file2.write((char *)pkt.header.raw, 6);
                    idk_file2.write((char *)pkt.payload.data(), pkt.payload.size());

                    repackBytesTo12bits(&pkt.payload[31], 5848 - 31, tirs_line);

                    for (int i = 0; i < 640; i++)
                        img_out1[lines * 1280 + i * 2 + 0] = tirs_line[i] << 4;
                    for (int i = 0; i < 640; i++)
                        img_out1[lines * 1280 + i * 2 + 1] = tirs_line[1941 + i] << 4;

                    for (int i = 0; i < 640; i++)
                        img_out2[lines * 1280 + i * 2 + 0] = tirs_line[647 + i] << 4;
                    for (int i = 0; i < 640; i++)
                        img_out2[lines * 1280 + i * 2 + 1] = tirs_line[2588 + i] << 4;

                    for (int i = 0; i < 640; i++)
                        img_out3[lines * 1280 + i * 2 + 0] = tirs_line[1294 + i] << 4;
                    for (int i = 0; i < 640; i++)
                        img_out3[lines * 1280 + i * 2 + 1] = tirs_line[3235 + i] << 4;

                    lines++;
                }
            }
        }
    }

    img_out1.save_png("TEST_TIRS_1.png");
    img_out2.save_png("TEST_TIRS_2.png");
    img_out3.save_png("TEST_TIRS_3.png");
#endif
}