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
#include "common/image/image.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();
    completeLoggerInit();

    std::ifstream data_in(argv[1], std::ios::binary);

    std::vector<uint8_t> img_vector;

    while (!data_in.eof())
    {
        uint8_t frm[70];
        data_in.read((char *)frm, 70);

        if (frm[4] == 0x02)
        {

            uint16_t hdr = frm[4 + 1] << 8 | frm[4 + 0];
            uint8_t data_size = frm[4 + 2];
            uint16_t msg_type = frm[4 + 4] << 8 | frm[4 + 3];
            uint16_t pkt_offset = frm[4 + 6] << 8 | frm[4 + 5];

            logger->trace(pkt_offset);

            int new_size = pkt_offset + 56;
            if (img_vector.size() < new_size)
                img_vector.resize(new_size);

            memcpy(&img_vector[pkt_offset], &frm[4 + 8], 56);
        }
    }

    image::Image<uint8_t> img_final;
    img_final.load_jpeg(img_vector.data(), img_vector.size());
    img_final.save_png("strato.png");
}