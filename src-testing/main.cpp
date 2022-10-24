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
#include "common/codings/viterbi/cc_encoder.h"
#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/randomization.h"
#include <fstream>

int main(int /*argc*/, char *argv[])
{
    initLogger();

    viterbi::CCEncoder vit_encoder(8192, 7, 2, {79, 109});
    viterbi::CCDecoder vit_decoder(8192, 7, 2, {79, 109});

    std::ofstream output_init("initial.vit");
    std::ofstream output_deco("decoded.vit");

    uint8_t input_frame[8192];
    uint8_t output_frame[8192];

    // i % 2 == 0 ? 1 : 0;

    uint8_t vit_encoded_buff[8192 * 2 * 100];

    for (int i = 0; i < 100; i++)
    {
        for (int y = 0; y < 8192; y++)
            input_frame[y] = rand() % 2;

        output_init.write((char *)input_frame, 8192);
        vit_encoder.work(input_frame, &vit_encoded_buff[i * 8192 * 2]);

        for (int y = 0; y < 8192 * 2; y++)
            (&vit_encoded_buff[i * 8192 * 2])[y] = ((&vit_encoded_buff[i * 8192 * 2])[y] ? 191 : 64) + (rand() % 200) - 100;
    }

    for (int i = 0; i < 100; i++)
    {
        vit_decoder.work(&vit_encoded_buff[i * 8192 * 2], output_frame);
        output_deco.write((char *)output_frame, 8192);
    }
}
