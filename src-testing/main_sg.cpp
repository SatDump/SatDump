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

#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"
#include "logger.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    uint8_t cadu[1024];

    std::ifstream data_in(argv[1]);
    std::ofstream data_ou(argv[2]);

    ccsds::ccsds_aos::Demuxer demuxer_vcid(884, false);

    std::vector<uint8_t> img_vec(520 * 520 * 2);
    int img_n = 0;

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1024);

        // Parse this transport frame
        ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid != 10)
            continue;

        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid.work(cadu);
        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
        {
            printf("APID %d\n", pkt.header.apid);

#if 1                                   // Img?
                                        // 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111
            if (pkt.header.apid == 577) // pkt.header.apid == 1440 || pkt.header.apid == 1443 || pkt.header.apid == 1444)
            {
                // int mkr = pkt.payload[21];

                //  printf("LEN %d %d MKR %d\n", pkt.header.sequence_flag, pkt.payload.size() + 6, mkr);

                {
                    pkt.payload.resize(5000 - 6);

                    //  data_ou.write((char *)pkt.header.raw, 6);
                    data_ou.write((char *)pkt.payload.data(), pkt.payload.size());
                }

                // data_ou.write((char *)pkt.payload.data() + 4320, pkt.payload.size() - 4320 - 4);
            }
            // else
            //     printf("APID %d\n", pkt.header.apid);
#endif

#if 0 // "MHS"
            if (pkt.header.apid == 1104)
            {
                printf("LEN %d %d\n", pkt.header.sequence_flag, pkt.payload.size() + 6);

                for (int i = 0; i < 1542; i++)
                {
                    uint16_t v = pkt.payload[28 + 2 * i + 0] << 8 | pkt.payload[28 + 2 * i + 1];
                    v += 32767;
                    img_vec.push_back(v);
                }

                pkt.payload.resize(4000 - 6, 0);

                data_ou.write((char *)pkt.header.raw, 6);
                data_ou.write((char *)pkt.payload.data(), pkt.payload.size());
            }
#endif
        }
    }

    // image::Image img(img_vec.data(), 16, 1542, img_vec.size() / 1542, 1);
    // image::save_png(img, "/tmp/test.png");
}