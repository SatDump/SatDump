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

#include "common/codings/differential/nrzm.h"

#include "common/dsp/utils/random.h"

int main(int argc, char *argv[])
{
    initLogger();

#if 0
    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t buffer1[1024];
    int8_t buffer2[8192];

    dsp::Random gaussian;

    while (!data_in.eof()) //&& lines_img < 500)
    {
        data_in.read((char *)buffer1, 1024);

        int bit2pos = 0;
        for (int i = 0; i < 1024; i++)
            for (int x = 6; x >= 0; x -= 2)
            {
                buffer2[bit2pos++] = (((buffer1[i] >> (x + 0)) & 1) ? 70 : -70) + gaussian.gasdev() * 10;
                buffer2[bit2pos++] = (((buffer1[i] >> (x + 1)) & 1) ? 70 : -70) + gaussian.gasdev() * 10;
            }

        data_out.write((char *)buffer2, bit2pos);
    }
#elif 1
    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t buffer1[1024];
    uint8_t buffer2[8192];
    uint8_t buffer3[8192];
    uint8_t buffer4[8192];

    diff::QPSKDiff diff;

    while (!data_in.eof()) //&& lines_img < 500)
    {
        data_in.read((char *)buffer1, 1024);

        int bit2pos = 0;
        for (int i = 0; i < 1024; i++)
        {
            for (int x = 6; x >= 0; x -= 2)
            {
                buffer2[bit2pos++] = ((buffer1[i] >> (x + 1)) & 1) << 1 |
                                     ((buffer1[i] >> (x + 0)) & 1);
            }
        }

        diff.work(buffer2, bit2pos, buffer3);

        for (int i = 0; i < 8192; i++)
            buffer4[i / 8] = buffer4[i / 8] << 1 | buffer3[i];

        data_out.write((char *)buffer4, 1024);
    }
#else
    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t buffer1[1024];
    uint8_t buffer2[8192];
    uint8_t buffer3[8192];
    uint8_t buffer4[8192];

    diff::NRZMDiff diff;

    while (!data_in.eof()) //&& lines_img < 500)
    {
        data_in.read((char *)buffer1, 1024);

        int bit2pos = 0;
        for (int i = 0; i < 1024; i++)
        {
            for (int x = 6; x >= 0; x -= 2)
            {
                buffer2[bit2pos++] = ((buffer1[i] >> (x + 1)) & 1); //<< 1 |
                                                                    // ((buffer1[i] >> (x + 0)) & 1);
            }
        }

        diff.decode_bits(buffer2, bit2pos);

        for (int i = 0; i < 8192; i++)
            buffer4[i / 8] = buffer4[i / 8] << 1 | buffer2[i];

        data_out.write((char *)buffer4, 1024 / 2);
    }
#endif
}