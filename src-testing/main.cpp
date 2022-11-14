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
#include "common/ccsds/ccsds_1_0_proba/vcdu.h"
#include "common/ccsds/ccsds_1_0_proba/demuxer.h"
#include "common/ccsds/ccsds_1_0_proba/mpdu.h"
#include <fstream>

#include "common/image/image.h"
#include "common/repack.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::ifstream idk_file(argv[1]);

    std::ofstream idk_file2(argv[2]);

    uint8_t buffer1[331];

    std::vector<uint8_t> wip_pkt;
    std::vector<uint8_t> wip_picture;
    int img_cnt = 1;

    while (!idk_file.eof())
    {
        idk_file.read((char *)buffer1, 331);

        uint8_t frame_type = buffer1[5];

        printf("MARKER %d\n", frame_type);

        if (frame_type == 217)
        {
            // idk_file2.write((char *)&buffer1[0], 331);
            uint16_t ptr = buffer1[18] << 8 | buffer1[17];

            uint16_t ptr2 = buffer1[19]; // << 8 | buffer1[17];

            printf("PTR %d %d\n", ptr, ptr2);

#if 0
            int ptr3 = 0;

            if (ptr2 >= 198)
                ptr2 = ptr3 = 198;
            else
            {
                ptr3 = 198 - ptr2;
            }

            // if (ptr2 != 198)
            idk_file2.write((char *)&buffer1[21], 198); // 202 - 4);
#else
            if (ptr2 == 198)
            {
                wip_pkt.insert(wip_pkt.end(), &buffer1[21], &buffer1[21 + 198]);
            }
            else if (ptr2 < 198)
            {
                wip_pkt.insert(wip_pkt.end(), &buffer1[21], &buffer1[21 + ptr2]);
                wip_pkt.erase(wip_pkt.begin(), wip_pkt.begin() + 8);

                wip_picture.insert(wip_picture.end(), wip_pkt.begin(), wip_pkt.end());

                wip_pkt.clear();
                wip_pkt.insert(wip_pkt.end(), &buffer1[21 + ptr2], &buffer1[21 + 198]);
            }
#endif
        }
    }

    idk_file2.write((char *)wip_picture.data(), wip_picture.size());

    uint8_t sliding_header[4] = {0, 0, 0, 0};
    std::vector<uint8_t> wip_until_hdr;

    for (uint8_t b : wip_picture)
    {
        sliding_header[0] = sliding_header[1];
        sliding_header[1] = sliding_header[2];
        sliding_header[2] = sliding_header[3];
        sliding_header[3] = b;

        wip_until_hdr.push_back(b);

        if (sliding_header[0] == 0xFF && sliding_header[1] == 0xD8 && sliding_header[2] == 0xFF && sliding_header[3] == 0xE0)
        {
            std::ofstream raw_img("BlueWaker3_Cam_" + std::to_string(img_cnt++) + ".jpg", std::ios::binary);
            raw_img.put(0xFF);
            raw_img.put(0xD8);
            raw_img.put(0xFF);
            raw_img.write((char *)wip_until_hdr.data(), wip_until_hdr.size());
            raw_img.close();
            wip_until_hdr.clear();
        }
    }

    if (wip_until_hdr.size() > 0)
    {
        std::ofstream raw_img("BlueWaker3_Cam_" + std::to_string(img_cnt++) + ".jpg", std::ios::binary);
        raw_img.put(0xFF);
        raw_img.put(0xD8);
        raw_img.put(0xFF);
        raw_img.write((char *)wip_until_hdr.data(), wip_until_hdr.size());
        raw_img.close();
    }
}