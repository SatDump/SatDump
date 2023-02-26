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
#include "init.h"
#include <fstream>
#include "common/dsp/hier/gfsk_mod.h"
#include "common/dsp/file_sink.h"

#include "common/codings/randomization.h"

#include "common/dsp_source_sink/dsp_sample_sink.h"

int main(int argc, char *argv[])
{
    initLogger();
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSinks();
    std::vector<dsp::SinkDescriptor> all_sinks = dsp::getAllAvailableSinks();

    std::shared_ptr<dsp::DSPSampleSink> actual_sink;

    for (auto s : all_sinks)
    {
        if (s.sink_type == "hackrf")
        {
            actual_sink = dsp::getSinkFromDescriptor(s);
            break;
        }
    }

    std::ifstream input_frm(argv[1]);

    uint8_t cadu[1024];

    std::shared_ptr<dsp::stream<float>> input_vco = std::make_shared<dsp::stream<float>>();

    dsp::GFSKMod gfsk_mod(input_vco, 1.0, 0.35);

    actual_sink->open();
    actual_sink->set_samplerate(8e6);
    actual_sink->set_frequency(100e6);
    actual_sink->set_settings(nlohmann::json::parse("{\"lna_gain\":20,\"vga_gain\":20}"));
    actual_sink->start(gfsk_mod.output_stream);
    // dsp::FileSinkBlock file_sink_blk(gfsk_mod.output_stream);

    gfsk_mod.start();
    // file_sink_blk.start();
    //  file_sink_blk.set_output_sample_type(dsp::CF_32);
    //  file_sink_blk.start_recording("test", 2e6);

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

    gfsk_mod.stop();
    actual_sink->stop();
    actual_sink->close();

    logger->critical("DONE!!");
}