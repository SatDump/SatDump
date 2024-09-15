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
#include <iostream>
#include "common/image/image.h"
#include <iomanip>
#include "common/image/io.h"

#include "common/image/processing.h"

int main(int argc, char *argv[])
{
    std::ifstream in;
    // in.open("custom_MPSK_test.cadu");
    in.open(argv[1]);
    uint8_t cadu[1279];
    std::ofstream out("test.bin");
    ccsds::ccsds_tm::Demuxer demuxer_vcid3(1109, false);
    initLogger();

    image::Image test_img(16, 40, 1000, 1);
    int line = 0;
    std::vector<uint16_t> channels[19];

    std::vector<uint8_t> full_payload;

    while (!in.eof())
    {
        in.read((char *)&cadu, 1279);
        ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

        if (vcdu.vcid == 3) // data
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 100 && pkt.payload.size() > 17)
                {
                    if (pkt.header.sequence_flag == 0b01)
                    {
                        if (full_payload.size() > 0)
                        {
                            logger->info(full_payload.size());

                            full_payload.resize(7599);
                            out.write((char *)full_payload.data(), 7599);

                            for (int c = 0; c < 145; c++)
                                for (int i = 0; i < 19; i++)
                                    channels[i].push_back((full_payload[1799 + (c * 40) + (i * 2) + 1] << 8) | full_payload[1799 + (c * 40) + (i * 2) + 0]);
                            line++;

                            full_payload.clear();
                        }

                        full_payload.insert(full_payload.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);
                    }
                    else
                        full_payload.insert(full_payload.end(), pkt.payload.begin() + 17, pkt.payload.end() - 2);

                    //  if (pkt.payload.size() >= 2067)
                    {
                        // if ((pkt.payload[17] >> 5) == 0b00000100)
                        // {
                        //   logger->info(pkt.payload.size());
                        //   out.write((char *)pkt.header.raw, 6);
                        //   pkt.payload.resize(2067);
                        //   out.write((char *)pkt.payload.data(), 2067);

                        //    for (int c = 0; c < 10; c++)
                        //    {
                        /*
                        for (int i = 0; i < 19; i++)
                        {
                            channels[c].push_back((pkt.payload[19 + i + 40*c] << 8) & pkt.payload[18 + i + 40*c]);
                        }
                        */
                        //         int i = 11;
                        // test_img.draw_pixel(c, line, {static_cast<double>((pkt.payload[19 + i + 40*c] << 8) & pkt.payload[18 + i + 40*c])});
                        //         channels[0].push_back((pkt.payload[19 + i + 40 * c] << 8) & pkt.payload[18 + i + 40 * c]);
                        //          line++;
                        //     }
                        //    }
                    }
                }
            }
        }
    }
    out.close();
    in.close();

    for (int i = 0; i < 19; i++)
    {
        test_img = image::Image(channels[i].data(), 16, 145, line, 1);
        image::equalize(test_img);
        image::save_png(test_img, "test" + std::to_string(i + 1) + ".png");
    }
}