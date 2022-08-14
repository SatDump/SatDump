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
#include "common/dsp/vco.h"
#include "common/dsp/firdes.h"
#include "common/dsp/rational_resampler.h"
#include "common/dsp/file_sink.h"

#include "common/codings/randomization.h"

int main(int argc, char *argv[])
{
    initLogger();
    std::ifstream input_frm(argv[1]);

    uint8_t cadu[1024];

    std::shared_ptr<dsp::stream<float>> input_vco = std::make_shared<dsp::stream<float>>();

    dsp::FFRationalResamplerBlock gaussian_fir(input_vco, 2, 1, dsp::firdes::convolve(dsp::firdes::gaussian(1, 2, 0.35, 31), {1, 1}));
    dsp::VCOBlock vco_block(gaussian_fir.output_stream, 1.0 /*(M_PI / 4.0)*/, 1);

    dsp::FileSinkBlock file_sink_blk(vco_block.output_stream);

    gaussian_fir.start();
    vco_block.start();
    file_sink_blk.start();
    file_sink_blk.set_output_sample_type(dsp::IS_8);
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
