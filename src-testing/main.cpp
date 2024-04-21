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

#include "common/ccsds/ccsds_weather/demuxer.h"
#include "common/ccsds/ccsds_weather/vcdu.h"

#include "common/repack.h"
#include "common/image/image.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_ou(argv[2], std::ios::binary);

    uint8_t cadu[1024];

    ccsds::ccsds_weather::Demuxer demuxer_vcid0(1010, true, 0);

    std::vector<uint8_t> wip_large_pkt;

    std::vector<uint16_t> wip_image_bufs[17];

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1024);

        // Parse this transport frame
        ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

        //   printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 35) // HK/TM
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                // printf("APID %d\n", pkt.header.apid);
                if (pkt.header.apid == 1005)
                {
                    //  printf("LEN %d\n", pkt.header.packet_length + 6);

                    if (pkt.header.sequence_flag == 0b01)
                    {
                        if (wip_large_pkt.size() > 0)
                        {
                            wip_large_pkt.resize(24798);
                            data_ou.write((char *)wip_large_pkt.data(), wip_large_pkt.size());

                            uint16_t bufline[12133];
                            repackBytesTo16bits(&wip_large_pkt[524], 24266, (uint16_t *)bufline);
                            for (int i = 0; i < 17; i++)
                                wip_image_bufs[i].insert(wip_image_bufs[i].end(), &bufline[714 * i], &bufline[714 * i + 571]);
                        }
                        wip_large_pkt.clear();

                        wip_large_pkt.insert(wip_large_pkt.end(), pkt.payload.begin(), pkt.payload.end());
                    }
                    else if (pkt.header.sequence_flag == 0b00)
                    {
                        wip_large_pkt.insert(wip_large_pkt.end(), pkt.payload.begin(), pkt.payload.end());
                    }
                    else if (pkt.header.sequence_flag == 0b10)
                    {
                        wip_large_pkt.insert(wip_large_pkt.end(), pkt.payload.begin(), pkt.payload.end());
                        if (wip_large_pkt.size() > 0)
                        {
                            wip_large_pkt.resize(24798);
                            data_ou.write((char *)wip_large_pkt.data(), wip_large_pkt.size());

                            uint16_t bufline[12133];
                            repackBytesTo16bits(&wip_large_pkt[524], 24266, (uint16_t *)bufline);
                            for (int i = 0; i < 17; i++)
                                wip_image_bufs[i].insert(wip_image_bufs[i].end(), &bufline[714 * i], &bufline[714 * i + 571]);
                        }
                        wip_large_pkt.clear();
                    }
                }
            }
        }
    }

    for (int i = 0; i < 17; i++)
    {
        image::Image<uint16_t> img(571, wip_image_bufs[i].size() / 571, 1);

        for (size_t ii = 0; ii < img.size(); ii++)
            img[ii] = wip_image_bufs[i][ii] - 32768;

        img.equalize();
        img.save_png("WIP__WSF_M_" + std::to_string(i + 1) + ".png");
    }
}