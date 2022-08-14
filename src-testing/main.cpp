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
#include "common/dsp/hier/gfsk_mod.h"
#include "common/dsp/file_sink.h"

#include "common/codings/randomization.h"

int main(int argc, char *argv[])
{
    initLogger();
    std::ifstream input_frm(argv[1]);

    uint8_t cadu[1024];

    std::shared_ptr<dsp::stream<float>> input_vco = std::make_shared<dsp::stream<float>>();

    dsp::GFSKMod gfsk_mod(input_vco, 1.0, 0.35);

    dsp::FileSinkBlock file_sink_blk(gfsk_mod.output_stream);

    gfsk_mod.start();
    file_sink_blk.start();
    file_sink_blk.set_output_sample_type(dsp::CF_32);
    file_sink_blk.start_recording("test", 2e6);

    while (!input_frm.eof())
    {
        input_frm.read((char *)cadu, 1024);

        derand_ccsds(&cadu[4], 1020);

        for (int i = 0; i < 8192; i++)
        {
            bool bit = (cadu[i / 8] >> (7 - (i % 8))) & 1;
            input_vco->writeBuf[i] = bit ? 1 : -1;
        }

        input_vco->swap(8192);
    }

    logger->critical("DONE!!");
}
