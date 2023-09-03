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
#include <cstdint>
#include "common/codings/deframing/bpsk_ccsds_deframer.h"

#include "common/utils.h"

#include "common/codings/randomization.h"

#include "common/codings/reedsolomon/reedsolomon.h"

#include "common/codings/differential/qpsk_diff.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t buffer1[1024];
    uint8_t buffer2[8192];
    uint8_t buffer3[8192];
    uint8_t buffer4[1024];

    diff::QPSKDiff diff;

    while (!data_in.eof()) //&& lines_img < 500)
    {
        data_in.read((char *)buffer1, 1024);

        int bit2pos = 0;
        for (int i = 0; i < 1024; i++)
            for (int x = 6; x >= 0; x -= 2)
                buffer2[bit2pos++] = ((buffer1[i] >> (x + 1)) & 1) << 1 | ((buffer1[i] >> (x + 0)) & 1);

        diff.work(buffer2, bit2pos, buffer3);

        // if ((cadu[24] >> 5) == 0b000)
        // {
        //     if (cadu[25] == 0) // cadu[25] == 0x95 && cadu[26] == 0x73)
        //     {
        //         data_out.write((char *)cadu, 2044);
        //     }
        // }

        // printf("%d\n", bit2pos);

        for (int i = 0; i < 1024; i++)
            buffer4[i] = buffer3[i * 8 + 0] << 7 |
                         buffer3[i * 8 + 1] << 6 |
                         buffer3[i * 8 + 2] << 5 |
                         buffer3[i * 8 + 3] << 4 |
                         buffer3[i * 8 + 4] << 3 |
                         buffer3[i * 8 + 5] << 2 |
                         buffer3[i * 8 + 6] << 1 |
                         buffer3[i * 8 + 7] << 0;

        data_out.write((char *)buffer4, 1024);
    }
}