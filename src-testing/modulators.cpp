#include "modulators.h"
#include <memory>
#include "common/dsp/io/file_sink.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/demod/constellation.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/randomization.h"
#include "logger.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"

std::shared_ptr<dsp::stream<complex_t>> input_samples_dl = std::make_shared<dsp::stream<complex_t>>();
CCSDSMuxer muxer_dl(200000, 100000);

#include "common/codings/viterbi/cc_encoder.h"

void modulatorThread_dl()
{
    uint8_t *cadus = new uint8_t[1024 * 10000];
    uint8_t *cadus_bits = new uint8_t[1024 * 10000];
    uint8_t *cadus_encoded = new uint8_t[1024 * 10000];

    dsp::constellation_t bpsk(dsp::BPSK);
    dsp::constellation_t qpsk(dsp::QPSK);

    reedsolomon::ReedSolomon rs(reedsolomon::RS223);

    viterbi::CCEncoder cc_enc(8192, 7, 2, {109, 79});

    while (true)
    {
        int cnt = 5;
        muxer_dl.work(cadus, cnt);

        int in_samples = 0;

        for (int c = 0; c < cnt; c++)
        {
            rs.encode_interlaved(&cadus[c * 1024 + 4], true, 4);
            derand_ccsds(&cadus[c * 1024 + 4], 1020);

            uint8_t *cadu = &cadus[c * 1024];
            for (int bit = 0; bit < 8192; bit++)
                cadus_bits[c * 8192 + bit] = (cadu[bit / 8] >> (7 - (bit % 8))) & 1;
        }

        cc_enc.work(cadus_bits, cadus_encoded, cnt * 8192);

        for (int i = 0; i < cnt * 8192; i++)
        {
#if 1
            input_samples_dl->writeBuf[in_samples++] = bpsk.mod(cadus_encoded[i * 2 + 1] & 1) * 0.5;
            input_samples_dl->writeBuf[in_samples++] = bpsk.mod(cadus_encoded[i * 2 + 0] & 1) * 0.5;
#else
            input_samples_dl->writeBuf[in_samples++] =
                qpsk.mod(cadus_encoded[i * 2 + 0] << 1 | cadus_encoded[i * 2 + 1]);
#endif
        }

        input_samples_dl->swap(in_samples);

        // logger->info("LRPT BUFF");
        // std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    delete[] cadus;
}

std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> resamp_blk_dl;
std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> resamp_blk_dl2;
// std::shared_ptr<dsp::FileSinkBlock> file_sink_blk_dl;
std::shared_ptr<dsp::DSPSampleSink> sink_ptr;
std::thread muxerThread_dl;
void boot_dl_modulator()
{
    resamp_blk_dl = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(input_samples_dl, 2, 1, dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.6, 31));
    resamp_blk_dl2 = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(resamp_blk_dl->output_stream, 1e6, 256e3);
    // file_sink_blk_dl = std::make_shared<dsp::FileSinkBlock>(resamp_blk_dl->output_stream);

    dsp::registerAllSinks();

    auto all_sinks = dsp::getAllAvailableSinks();

    logger->critical(all_sinks[0].name);

    sink_ptr = dsp::getSinkFromDescriptor(all_sinks[0]);
    sink_ptr->open();

    sink_ptr->set_frequency(240305e4);
    sink_ptr->set_samplerate(1e6);

    nlohmann::json prms;
    prms["gain_mode"] = 0;
    prms["general_gain"] = 60;

    sink_ptr->set_settings(prms);

    resamp_blk_dl->start();
    resamp_blk_dl2->start();
    // file_sink_blk_dl->start();
    sink_ptr->start(resamp_blk_dl2->output_stream);

    logger->info("STARTED LRPT");

    // file_sink_blk_dl->set_output_sample_type(dsp::CF_32);
    // file_sink_blk_dl->start_recording("test_modulator_mux", 2e6);

    logger->info("STARTED LRPT REC");

    muxerThread_dl = std::thread(&modulatorThread_dl);

    logger->info("STARTED LRPT THREAD");
}
