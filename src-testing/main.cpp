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
#include <cmath>
#include "common/image/image.h"
#include "common/repack.h"
#include "common/codings/randomization.h"
#include "common/image/image.h"
#include <filesystem>

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t cadu[1264];

    ccsds::ccsds_weather::Demuxer demuxer_vcid1(1092, false);

    int lines = 0;
    int pkts = 0;
    std::vector<image::Image<uint16_t>> img_out;
    for (int i = 0; i < 20; i++)
        img_out.push_back(image::Image<uint16_t>(243, 50, 1));

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)cadu, 1264);

        //  derand_ccsds(&cadu[4], 1264 - 4);

        // Parse this transport frame
        ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 17)
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                // logger->critical(pkt.header.apid);
                // logger->critical(pkt.header.sequence_flag);
                // logger->critical(pkt.header.secondary_header_flag);
                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 1576 /*288*/)
                {
#if 1
                    if (pkts < 20)
                    {
                        int nch = 20;
                        int npx = 25;
                        for (int i = 0; i < npx; i++)
                        {
                            for (int c = 0; c < 20; c++)
                            // int c = 4;
                            {
                                int off = 10 + 2 * (i * nch + c);

                                uint16_t val = pkt.payload[off + 0] << 8 | pkt.payload[off + 1];

                                bool sign = (val >> 11) & 1;
                                val = val & 0b11111111111;

                                if (!sign)
                                    val = 2048 + val;
                                // else
                                //     val = 2048 - val;

                                int px = pkts * npx + i;
                                if (px < 243)
                                    img_out[c][lines * 243 + px] = val << 4;
                            }
                        }
                    }
#else
                    for (int i = 0; i < 500; i++)
                        img_out[lines * 5000 + (pkts)*500 + i] = pkt.payload[10 + 2 * i + 0] << 8 | pkt.payload[10 + 2 * i + 1];
#endif

                    pkts++;

                    if (pkt.header.sequence_flag == 0b01)
                    {
                        lines++;
                        pkts = 0;
                    }

                    {
                        printf("LEN %d\n", pkt.payload.size());
                        pkt.payload.resize(1018);
                        data_out.write((char *)pkt.header.raw, 6);
                        data_out.write((char *)pkt.payload.data(), 1018);
                    }
                }
            }
        }
    }

    for (int i = 0; i < 20; i++)
    {
        img_out[i].crop(0, 0, 243, lines);
        img_out[i].equalize();
        img_out[i].save_png("idk_" + std::to_string(i) + ".png");
    }
}