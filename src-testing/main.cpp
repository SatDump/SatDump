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

#include "common/ccsds/ccsds_time.h"
#include "common/utils.h"
#include <cmath>
#include <filesystem>

image::Image images_idk[11];

int cimage = 0;

void saveImages()
{
    cimage++;

    std::string directory = (std::string) "msg_out/" + "msg_" + std::to_string(cimage) + "/";

    if (!std::filesystem::exists(directory))
        std::filesystem::create_directories(directory);

    image::Image img321(16, 3834, 4482, 3);

    for (int i = 0; i < 11; i++)
    {
        images_idk[i].mirror(true, true);
        image::save_png(images_idk[i], directory + "test_msg" + std::to_string(cimage) + "_" + std::to_string(i + 1) + ".png");
        // printf("SAVING %d\n", timeReadable.tm_min);

        if (i == 2)
            img321.draw_image(0, images_idk[i], 18, 0);
        if (i == 1)
            img321.draw_image(1, images_idk[i], -18, 0);
        if (i == 0)
            img321.draw_image(2, images_idk[i], 0, 0);

        images_idk[i].fill(0);
    }

    image::save_png(img321, directory + "test_msg" + std::to_string(cimage) + "_" + "321" + ".png");
}

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

    int lines = 0, rlines = 0;
    //
    for (int i = 0; i < 11; i++)
        images_idk[i] = image::Image(16, 3834, 4482, 1);

    int cpayload = 0;

    double last_time = 0;

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1279);

        // Parse this transport frame
        ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

        // printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 0)
        {
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                // printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 2046)
                {
                    cpayload = pkt.header.packet_sequence_count % 16;

                    // printf("LEN %d %d\n", pkt.payload.size() + 6, cpayload);

                    if (cpayload < 11) //|| cpayload == 1)
                    {
                        double ltime = ccsds::parseCCSDSTimeFull(pkt, 17720, 65535, 1e100); //- 60 * 3;

                        if (cpayload == 0)
                        {
                            printf("Time %s - %f\n", timestamp_to_string(ltime).c_str(), ltime - last_time);
                            last_time = ltime;

                            time_t tttime = ltime;
                            std::tm timeReadable = *gmtime(&tttime);

                            if (rlines++ > 500 && (timeReadable.tm_min == 0 || timeReadable.tm_min == 15 || timeReadable.tm_min == 30 || timeReadable.tm_min == 45))
                            {
                                rlines = 0;
                                last_time = ltime;
                                saveImages();
                            }
                        }

                        lines = fmod(ltime, 15 * 60) / (300 / 1494.0);

                        uint16_t tmp_buf[15000];
                        repackBytesTo10bits(&pkt.payload[3], pkt.payload.size(), tmp_buf); //&image_idk[lines * image_idk.width()]);
                        for (int c = 0; c < 3; c++)
                        {
                            for (int v = 0; v < 3834; v++)
                                images_idk[cpayload].set(lines * images_idk[cpayload].width() + v, tmp_buf[(c) * 3834 + v]);
                            for (int i = 0; i < images_idk[cpayload].width(); i++)
                                images_idk[cpayload].set(lines * images_idk[cpayload].width() + i, images_idk[cpayload].get(lines * images_idk[cpayload].width() + i) << 6);
                            lines++;
                        }

                        pkt.payload.resize(14392 - 6);
                        data_ou.write((char *)pkt.header.raw, 6);
                        data_ou.write((char *)pkt.payload.data(), pkt.payload.size());
                    }

                    cpayload++;
                }
            }
        }

        /*if (vcdu.vcid == 1)
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
        }*/
    }

    saveImages();
}