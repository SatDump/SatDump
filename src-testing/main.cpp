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
#include "common/ccsds/ccsds_1_0_proba/vcdu.h"
#include "common/ccsds/ccsds_1_0_proba/demuxer.h"
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

    uint8_t cadu[1279];

    ccsds::ccsds_1_0_proba::Demuxer demuxer_vcid1(1103, false);

    std::vector<uint8_t> wip_payload;

    int nb_img = 0;

    int offset = 11; // std::stoi(argv[3]);

    std::vector<uint16_t> img_stuff;

    std::vector<uint16_t> img_stuff_51;
    std::vector<uint16_t> img_stuff_56;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)cadu, 1279);

        //  derand_ccsds(&cadu[4], 1279 - 4);

        // Parse this transport frame
        ccsds::ccsds_1_0_proba::VCDU vcdu = ccsds::ccsds_1_0_proba::parseVCDU(cadu);

        // printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 1)
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                // logger->critical(pkt.header.apid);
                // logger->critical(pkt.header.sequence_flag);
                // logger->critical(pkt.header.secondary_header_flag);
                // printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 100 /*288*/)
                {

#if 1
                    pkt.payload.resize(2000 - 6);
                    data_out.write((char *)pkt.header.raw, 6);
                    data_out.write((char *)pkt.payload.data(), 2000 - 6);

                    for (int i = 0; i < 511; i++)
                    {
                        img_stuff.push_back(pkt.payload[11 + i * 2 + 1] << 8 | pkt.payload[11 + i * 2 + 0]);
                    }

#else
                    if (pkt.header.sequence_flag == 0b01 && pkt.payload.size() > 512)
                    {
                        logger->critical(pkt.payload[10]);
                        wip_payload.clear();
                        wip_payload.insert(wip_payload.end(), &pkt.payload[64], &pkt.payload[pkt.payload.size() - 2]);
                    }
                    else if (pkt.header.sequence_flag == 0b00)
                    {
                        wip_payload.insert(wip_payload.end(), &pkt.payload[offset], &pkt.payload[pkt.payload.size() - 2]);
                    }
                    else if (pkt.header.sequence_flag == 0b10)
                    {
                        wip_payload.insert(wip_payload.end(), &pkt.payload[offset], &pkt.payload[pkt.payload.size() - 2]);

                        nb_img++;

                        std::ofstream output_jpg("/tmp/idk.jpg");
                        output_jpg.write((char *)wip_payload.data(), wip_payload.size());
                        std::string command = "ffmpeg -hide_banner -loglevel panic -y -i /tmp/idk.jpg ./mats_idk_" + std::to_string(nb_img) + ".png";

                        system(command.c_str());

                        if (std::filesystem::exists("./mats_idk_" + std::to_string(nb_img) + ".png"))
                        {
                            image::Image<uint16_t> img;
                            img.load_png("./mats_idk_" + std::to_string(nb_img) + ".png");

                            if (img.width() == 56)
                            {

                                img_stuff_56.insert(img_stuff_56.end(), &img[0], &img[56]);
                                img_stuff_56.insert(img_stuff_56.end(), &img[56], &img[56 * 2]);
                            }
                            // else if (img.width() == 51)
                            // {
                            //     for (int x = 0; x < img.width(); x++)
                            //         for (int y = 0; y < img.height(); y++)
                            //             img_stuff_51.push_back(img[y * 51 + x]);
                            // }

                            if (img.width() != 10) // 10, 51, 53, 56
                                std::filesystem::remove("./mats_idk_" + std::to_string(nb_img) + ".png");
                        }

                        // image::Image<uint16_t> img;
                        // img.load_jpeg(wip_payload.data(), wip_payload.size());
                        // img.save_png("mats_idk_" + std::to_string(nb_img) + ".png");
                    }
                    /*  else if (pkt.header.sequence_flag == 0b11)
                      {
                          wip_payload.clear();
                          wip_payload.insert(wip_payload.end(), &pkt.payload[11], &pkt.payload[pkt.payload.size()]);

                          nb_img++;

                          std::ofstream output_jpg("/tmp/idk.jpg");
                          output_jpg.write((char *)wip_payload.data(), wip_payload.size());
                          std::string command = "ffmpeg -y -i /tmp/idk.jpg ./mats_idk_" + std::to_string(nb_img) + ".png";

                          system(command.c_str());

                          // image::Image<uint16_t> img;
                          // img.load_jpeg(wip_payload.data(), wip_payload.size());
                          // img.save_png("mats_idk_" + std::to_string(nb_img) + ".png");
                      }*/

                    if (pkt.header.sequence_flag == 0b11)
                    {
                        pkt.payload.resize(10000 - 6);
                        data_out.write((char *)pkt.header.raw, 6);
                        data_out.write((char *)pkt.payload.data(), 10000 - 6);
                    }
#endif
                }
            }
        }
    }

    // image::Image<uint16_t> test_img(img_stuff.data(), 511, img_stuff.size() / 511, 1);
    // test_img.save_img("idk_mats.png");

    image::Image<uint16_t> test_img_56(img_stuff_56.data(), 56, img_stuff_56.size() / 56, 1);
    test_img_56.save_img("idk_mats_scan_56.png");

    // image::Image<uint16_t> test_img_51(img_stuff_51.data(), 255, img_stuff_51.size() / 255, 1);
    // test_img_51.save_img("idk_mats_scan_51.png");
}
