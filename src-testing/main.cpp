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
#include <cstring>
#include "common/ccsds/ccsds_standard/demuxer.h"
#include "common/ccsds/ccsds_standard/vcdu.h"

#include "common/image/image.h"
#include "common/repack.h"

#include "common/image/io.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_ou(argv[2], std::ios::binary);

    int nchannel = std::stoi(argv[3]);

    uint8_t cadu[1279];

    // Demuxers
    ccsds::ccsds_standard::Demuxer demuxer_vcid0(1109, false, 0, 0);
    ccsds::ccsds_standard::Demuxer demuxer_vcid1(1109, false, 0, 0);

    int lines = 0;
    image::Image image_idk(16, 5000, 7000 * 3, 1);

    int cpayload = 0;

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1279);

        // Parse this transport frame
        ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 0)
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                // printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 2046)
                {
                    // printf("LEN %d %d\n", pkt.payload.size() + 6, cpayload);

                    if (pkt.header.packet_length + 7 == 9203)
                    {

                        cpayload = 0;
                    }

                    if (cpayload == nchannel) //|| cpayload == 1)
                    {
                        pkt.payload.resize(14392 - 6);
                        // data_ou.write((char *)pkt.header.raw, 6);
                        // data_ou.write((char *)pkt.payload.data(), pkt.payload.size());

                        uint16_t tmp_buf[15000];
                        repackBytesTo10bits(&pkt.payload[3], pkt.payload.size(), tmp_buf); //&image_idk[lines * image_idk.width()]);
                        for (int c = 0; c < 3; c++)
                        {
                            for (int v = 0; v < 3834; v++)
                                image_idk.set(lines * image_idk.width() + v, tmp_buf[(c) * 3834 + v]);
                            for (int i = 0; i < image_idk.width(); i++)
                                image_idk.set(lines * image_idk.width() + i, image_idk.get(lines * image_idk.width() + i) << 6);
                            lines++;
                        }
                    }

                    cpayload++;
                }
            }
        }

        if (vcdu.vcid == 1)
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 2045)
                {
                    printf("LEN %d %d\n", pkt.payload.size() + 6, cpayload);

                    pkt.payload.resize(10000 - 6);
                    data_ou.write((char *)pkt.header.raw, 6);
                    data_ou.write((char *)pkt.payload.data(), pkt.payload.size());
                }
            }
        }
    }

    image::save_png(image_idk, "test_msg" + std::to_string(nchannel) + ".png");
}