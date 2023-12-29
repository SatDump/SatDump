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
#include "common/codings/differential/qpsk_diff.h"
#include "common/simple_deframer.h"
#include "common/utils.h"

uint8_t reverseBits(uint8_t byte)
{
    byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
    byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
    byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
    return byte;
}

int main(int /*argc*/, char *argv[])
{
    initLogger();
    completeLoggerInit();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t buffer1[1024];
    uint8_t buffer2[8192];
    uint8_t buffer3[8192];
    uint8_t buffer4[8192];

    diff::QPSKDiff diff;

    // def::SimpleDeframer kmss_deframer(0x7BAB8E8341FD1B0E, 64, 491520, 0, false);
    // def::SimpleDeframer kmss_deframer(0x1995a6559596a5aa, 64, 491520, 0, false);
    // def::SimpleDeframer kmss_deframer(0xdd5f755d5d7f5ff5, 64, 491520, 0, false);
    def::SimpleDeframer kmss_deframer(0x56a956a956a96681, 64, 491520, 0, false);

    double filesize_full = getFilesize(argv[1]);

    double last_time = time(0);

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

#if 0
        data_out.write((char *)buffer4, 1024);
#else
        std::vector<std::vector<uint8_t>> vf = kmss_deframer.work(buffer4, 1024);
        for (auto v : vf)
        {
            // for (auto &b : v)
            //     b ^= 0b10101010 ^ 0xFF;

            // 0xb0 4f a5 0f f0 1f b5 4a
            for (int i = 0; i < v.size(); i += 2)
            {
                // v[i + 0] ^= 0b11000000;
                // v[i + 1] ^= 0b00111111;

                // v[i + 0] ^= 0x95;
                // v[i + 1] ^= 0x6a;
              //  v[i + 0] ^= 0x56;
              //  v[i + 1] ^= 0xa9;

               // v[i + 0] = reverseBits(v[i + 0]);
               // v[i + 1] = reverseBits(v[i + 1]);

                // v[i + 0] ^= 0xb0;
                // v[i + 1] ^= 0x4f;
                // v[i + 2] ^= 0xa5;
                // v[i + 3] ^= 0x0f;
                // v[i + 4] ^= 0xf0;
                // v[i + 5] ^= 0x1f;
                // v[i + 6] ^= 0xb5;
                // v[i + 7] ^= 0x4a;
            }
#if 0
            for (int i = 0; i < v.size(); i += 1)
            {
                bool bit0 = (v[i] >> 7) & 1;
                bool bit1 = (v[i] >> 6) & 1;
                bool bit2 = (v[i] >> 5) & 1;
                bool bit3 = (v[i] >> 4) & 1;
                bool bit4 = (v[i] >> 3) & 1;
                bool bit5 = (v[i] >> 2) & 1;
                bool bit6 = (v[i] >> 1) & 1;
                bool bit7 = (v[i] >> 0) & 1;
                // v[i] = bit1 << 7 | bit0 << 6 |
                //        bit3 << 5 | bit2 << 4 |
                //        bit5 << 3 | bit4 << 2 |
                //        bit7 << 1 | bit6 << 0;
                v[i] = bit7 << 7 | bit6 << 6 |
                       bit5 << 5 | bit4 << 4 |
                       bit3 << 3 | bit2 << 2 |
                       bit1 << 1 | bit0 << 0;
            }
#endif

            data_out.write((char *)v.data(), 61440);
            logger->info("FRAME!");
        }

        if (time(0) - last_time > 2)
        {
            logger->trace("%.2f", (double(data_in.tellg()) / double(filesize_full)) * 100);
            last_time = time(0);
        }
#endif
    }
}