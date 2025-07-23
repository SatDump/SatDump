#include "common/dsp/path/splitter.h"
#include "common/dsp_source_sink/format_notated.h"
#include "common/utils.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/path/splitter.h"
#include "dsp/utils/freq_shift.h"
#include "init.h"
#include "logger.h"
#include "utils/time.h"

#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/filter/fft_filter.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/utils/freq_shift.h"
#include <complex.h>
#include <string>

#define benchmarkBlock(T_IN, T_OUT, T_BLK, DURATION, BLOCKNAME, MULTIPICATOR, ...)                                                                                                                     \
    {                                                                                                                                                                                                  \
        std::shared_ptr<dsp::stream<T_IN>> input_stream = std::make_shared<dsp::stream<T_IN>>();                                                                                                       \
                                                                                                                                                                                                       \
        T_BLK *block_to_test = new T_BLK(input_stream, __VA_ARGS__);                                                                                                                                   \
                                                                                                                                                                                                       \
        bool should_run = true;                                                                                                                                                                        \
        auto producer = [input_stream, &should_run]()                                                                                                                                                  \
        {                                                                                                                                                                                              \
            while (should_run)                                                                                                                                                                         \
            {                                                                                                                                                                                          \
                for (int i = 0; i < 8192 * 4; i++)                                                                                                                                                     \
                    input_stream->writeBuf[i] = T_IN(i / 1e5);                                                                                                                                         \
                input_stream->swap(8192 * 4);                                                                                                                                                          \
            }                                                                                                                                                                                          \
        };                                                                                                                                                                                             \
                                                                                                                                                                                                       \
        std::thread producer_th(producer);                                                                                                                                                             \
                                                                                                                                                                                                       \
        block_to_test->start();                                                                                                                                                                        \
                                                                                                                                                                                                       \
        double received = 0;                                                                                                                                                                           \
        double start_time = satdump::getTime();                                                                                                                                                        \
        double end_time = satdump::getTime();                                                                                                                                                          \
        while ((end_time - start_time) < DURATION)                                                                                                                                                     \
        {                                                                                                                                                                                              \
            int v = block_to_test->output_stream->read();                                                                                                                                              \
            received += v;                                                                                                                                                                             \
                                                                                                                                                                                                       \
            end_time = satdump::getTime();                                                                                                                                                             \
                                                                                                                                                                                                       \
            block_to_test->output_stream->flush();                                                                                                                                                     \
        }                                                                                                                                                                                              \
                                                                                                                                                                                                       \
        should_run = false;                                                                                                                                                                            \
        block_to_test->stop();                                                                                                                                                                         \
        input_stream->stopReader();                                                                                                                                                                    \
        input_stream->stopWriter();                                                                                                                                                                    \
                                                                                                                                                                                                       \
        if (producer_th.joinable())                                                                                                                                                                    \
            producer_th.join();                                                                                                                                                                        \
                                                                                                                                                                                                       \
        double throughput = received / (end_time - start_time);                                                                                                                                        \
        throughput *= MULTIPICATOR;                                                                                                                                                                    \
        logger->warn((std::string) "Performance (" + BLOCKNAME + ") %s", format_notated(throughput, "S/s").c_str());                                                                                   \
                                                                                                                                                                                                       \
        delete block_to_test;                                                                                                                                                                          \
    }

#include "dsp/agc/agc.h"
#include "dsp/filter/fir.h"

template <typename T>
void benchmarkNDSPBlock(std::shared_ptr<satdump::ndsp::Block> ptr, int duration, std::string blkname)
{
    satdump::ndsp::BlockIO istr;
    istr.fifo = std::make_shared<satdump::ndsp::DspBufferFifo>(4); // TODOREWORK FIFONUM?
    istr.name = "o";
    istr.type = satdump::ndsp::DSP_SAMPLE_TYPE_CF32;

    ptr->set_input(istr, 0);

    std::atomic<bool> should_run = true;
    auto producer = [&istr, &should_run]()
    {
        while (should_run)
        {
            satdump::ndsp::DSPBuffer nbuf = satdump::ndsp::DSPBuffer::newBufferSamples<T>(8192 * 4);
            for (int i = 0; i < 8192 * 4; i++)
                nbuf.getSamples<T>()[i] = T(i / 1e5);
            nbuf.size = 8192 * 4;
            istr.fifo->wait_enqueue(nbuf);
        }

        istr.fifo->wait_enqueue(satdump::ndsp::DSPBuffer::newBufferTerminator());
    };

    std::thread producer_th(producer);

    auto blk_out = ptr->get_output(0, 4 /*TODOREWORK*/);

    ptr->start();

    double received = 0;
    double start_time = satdump::getTime();
    double end_time = satdump::getTime();
    auto p2 = [&]()
    {
        while (true)
        {
            satdump::ndsp::DSPBuffer iblk;
            blk_out.fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
                break;

            received += iblk.size;

            end_time = satdump::getTime();

            iblk.free();
        }
    };
    std::thread consumer_th(p2);

    std::this_thread::sleep_for(std::chrono::seconds(duration));
    should_run = false;

    ptr->stop();
    if (producer_th.joinable())
        producer_th.join();
    if (consumer_th.joinable())
        consumer_th.join();

    double throughput = received / (end_time - start_time);
    //   throughput *= MULTIPICATOR;
    logger->warn((std::string) "Performance (" + blkname + ") %s", format_notated(throughput, "S/s").c_str());
}

int main(int argc, char *argv[])
{
    // We don't wanna spam with init this time around
    initLogger();
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    logger->debug("Benchmarking AGC...");
    benchmarkBlock(complex_t, complex_t, dsp::AGCBlock<complex_t>, 10, "AGC, Default", 1.0, /**/ 0.01, 1.0, 1.0, 65535);
    {
        auto p = std::make_shared<satdump::ndsp::AGCBlock<complex_t>>();
        p->p_rate = 0.01;
        p->p_reference = 1.0;
        p->p_gain = 1.0;
        p->p_max_gain = 65535;
        benchmarkNDSPBlock<complex_t>(p, 10, "AGC, New");
    }

    logger->debug("Benchmarking Costas Loop...");
    benchmarkBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 2", 1.0, /**/ 0.02, 2);
    benchmarkBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 4", 1.0, /**/ 0.02, 4);
    benchmarkBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 8", 1.0, /**/ 0.02, 8);

    logger->debug("Benchmarking FIR RRC...");
    // benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 1.2", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 1.2e6, 1e6, 0.35, 31));
    benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31));
    // benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 3.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 3, 1, 0.35, 31));
    benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 361 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361));
    {
        auto p = std::make_shared<satdump::ndsp::FIRBlock<complex_t>>();
        p->p_taps = dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31);
        p->p_buffer_size = 8192 * 8;
        benchmarkNDSPBlock<complex_t>(p, 10, "FIR New RRC, 31 taps, omega 2.0");
    }
    {
        auto p = std::make_shared<satdump::ndsp::FIRBlock<complex_t>>();
        p->p_taps = dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361);
        p->p_buffer_size = 8192 * 8;
        benchmarkNDSPBlock<complex_t>(p, 10, "FIR New RRC, 361 taps, omega 2.0");
    }

    // return 0;

    logger->debug("Benchmarking MM Recovery...");
    benchmarkBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 1.2", 1.0, /**/ 1.2, 0.1, 0.5, 0.01, 0.01);
    benchmarkBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 2.0", 1.0, /**/ 2.0, 0.1, 0.5, 0.01, 0.01);
    benchmarkBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 3.0", 1.0, /**/ 3.0, 0.1, 0.5, 0.01, 0.01);
    for (float v : {1.2, 2.0, 3.0})
    {
        auto p = std::make_shared<satdump::ndsp::MMClockRecoveryBlock<complex_t>>();
        p->set_cfg("omega", v);
        benchmarkNDSPBlock<complex_t>(p, 10, "MM NEW Recovery, omega " + std::to_string(v));
    }

    logger->debug("Benchmarking Splitter (1 out)...");
    // benchmarkBlock(complex_t, complex_t, dsp::SplitterBlock, 10, "Freq Shift, Default", 1.0);
    {
        auto p = std::make_shared<satdump::ndsp::SplitterBlock<complex_t>>();
        p->set_cfg("noutputs", 1);
        benchmarkNDSPBlock<complex_t>(p, 10, "Splitter, New");
    }

    logger->debug("Benchmarking Freq Shift...");
    benchmarkBlock(complex_t, complex_t, dsp::FreqShiftBlock, 10, "Freq Shift, Default", 1.0, /**/ 1e6, 100e3);
    {
        auto p = std::make_shared<satdump::ndsp::FreqShiftBlock>();
        p->set_cfg("samplerate", 1e6);
        p->set_cfg("freq_shift", 100e3);
        benchmarkNDSPBlock<complex_t>(p, 10, "Freq Shift, New");
    }

    logger->debug("Benchmarking Resamplers...");
    benchmarkBlock(complex_t, complex_t, dsp::SmartResamplerBlock<complex_t>, 10, "Smart Resampler, Dec 10", 10.0, /**/ 1, 10);
    benchmarkBlock(complex_t, complex_t, dsp::SmartResamplerBlock<complex_t>, 10, "Smart Resampler, Dec 100", 100.0, /**/ 1, 100);
    benchmarkBlock(complex_t, complex_t, dsp::RationalResamplerBlock<complex_t>, 10, "Polyphase Resampler, Dec 10", 10.0, /**/ 1, 10);
    benchmarkBlock(complex_t, complex_t, dsp::RationalResamplerBlock<complex_t>, 10, "Polyphase Resampler, Dec 100", 100.0, /**/ 1, 100);
}
