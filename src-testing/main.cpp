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
#include "common/codings/turbo/ccsds_turbo.h"
#include <fstream>

int main(int argc, char *argv[])
{
    initLogger();

    codings::turbo::CCSDSTurbo ccsds_turbo(codings::turbo::BASE_223, codings::turbo::RATE_1_2);

    logger->critical("Frame length {:d}", ccsds_turbo.frame_length());
    logger->critical("Codeword length {:d}", ccsds_turbo.codeword_length());

    uint8_t turbo_frame[223];
    uint8_t turbo_codeword[3576 / 8];
    std::ifstream ts_in("/home/alan/Downloads/sk8.ts");
    std::ofstream encoded_in("/home/alan/encoded_turbo.ts");

    float turbo_soft_frame[3576];

    while (!ts_in.eof())
    {
        ts_in.read((char *)turbo_frame, 188);

        ccsds_turbo.encode(turbo_frame, turbo_codeword);

        for (int i = 0; i < 3576; i++)
        {
            uint8_t bit = (turbo_codeword[i / 8] >> (7 - (i % 8))) & 1;
            turbo_soft_frame[i] = bit ? 1 : -1;
        }

        for (int i = 0; i < 100; i++)
        {
            int pos = rand() % 3576;
            turbo_soft_frame[pos] = -turbo_soft_frame[pos];
        }

        ccsds_turbo.decode(turbo_soft_frame, turbo_frame, 5);

        encoded_in.write((char *)turbo_frame, 188);
    }
}
