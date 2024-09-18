#include "logger.h"
#include "init.h"
#include "common/utils.h"
#include "common/dsp_source_sink/format_notated.h"

#include "common/dsp/filter/fir.h"
#include "common/dsp/filter/fft_filter.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/utils/freq_shift.h"
#include "common/dsp/resamp/smart_resampler.h"

#define benchmarkBlock(T_IN, T_OUT, T_BLK, DURATION, BLOCKNAME, MULTIPICATOR, ...)                                   \
    {                                                                                                                \
        std::shared_ptr<dsp::stream<T_IN>> input_stream = std::make_shared<dsp::stream<T_IN>>();                     \
                                                                                                                     \
        T_BLK *block_to_test = new T_BLK(input_stream, __VA_ARGS__);                                                 \
                                                                                                                     \
        bool should_run = true;                                                                                      \
        auto producer = [input_stream, &should_run]()                                                                \
        {                                                                                                            \
            while (should_run)                                                                                       \
            {                                                                                                        \
                for (int i = 0; i < 8192 * 4; i++)                                                                   \
                    input_stream->writeBuf[i] = 0;                                                                   \
                input_stream->swap(8192 * 4);                                                                        \
            }                                                                                                        \
        };                                                                                                           \
                                                                                                                     \
        std::thread producer_th(producer);                                                                           \
                                                                                                                     \
        block_to_test->start();                                                                                      \
                                                                                                                     \
        double received = 0;                                                                                         \
        double start_time = getTime();                                                                               \
        double end_time = getTime();                                                                                 \
        while ((end_time - start_time) < DURATION)                                                                   \
        {                                                                                                            \
            int v = block_to_test->output_stream->read();                                                            \
            received += v;                                                                                           \
                                                                                                                     \
            end_time = getTime();                                                                                    \
                                                                                                                     \
            block_to_test->output_stream->flush();                                                                   \
        }                                                                                                            \
                                                                                                                     \
        should_run = false;                                                                                          \
        block_to_test->stop();                                                                                       \
        input_stream->stopReader();                                                                                  \
        input_stream->stopWriter();                                                                                  \
                                                                                                                     \
        if (producer_th.joinable())                                                                                  \
            producer_th.join();                                                                                      \
                                                                                                                     \
        double throughput = received / (end_time - start_time);                                                      \
        throughput *= MULTIPICATOR;                                                                                  \
        logger->warn((std::string) "Performance (" + BLOCKNAME + ") %s", format_notated(throughput, "S/s").c_str()); \
                                                                                                                     \
        delete block_to_test;                                                                                        \
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

    logger->debug("Benchmarking Costas Loop...");
    benchmarkBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 2", 1.0, /**/ 0.02, 2);
    benchmarkBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 4", 1.0, /**/ 0.02, 4);
    benchmarkBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 8", 1.0, /**/ 0.02, 8);

    logger->debug("Benchmarking FIR RRC...");
    // benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 1.2", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 1.2e6, 1e6, 0.35, 31));
    benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31));
    // benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 3.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 3, 1, 0.35, 31));
    benchmarkBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 361 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361));

    logger->debug("Benchmarking MM Recovery...");
    benchmarkBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 1.2", 1.0, /**/ 1.2, 0.1, 0.5, 0.01, 0.01);
    benchmarkBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 2.0", 1.0, /**/ 2.0, 0.1, 0.5, 0.01, 0.01);
    benchmarkBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 3.0", 1.0, /**/ 3.0, 0.1, 0.5, 0.01, 0.01);

    logger->debug("Benchmarking Freq Shift...");
    benchmarkBlock(complex_t, complex_t, dsp::FreqShiftBlock, 10, "Freq Shift, Default", 1.0, /**/ 1e6, 100e3);

    logger->debug("Benchmarking Resamplers...");
    benchmarkBlock(complex_t, complex_t, dsp::SmartResamplerBlock<complex_t>, 10, "Smart Resampler, Dec 10", 10.0, /**/ 1, 10);
    benchmarkBlock(complex_t, complex_t, dsp::SmartResamplerBlock<complex_t>, 10, "Smart Resampler, Dec 100", 100.0, /**/ 1, 100);
    benchmarkBlock(complex_t, complex_t, dsp::RationalResamplerBlock<complex_t>, 10, "Polyphase Resampler, Dec 10", 10.0, /**/ 1, 10);
    benchmarkBlock(complex_t, complex_t, dsp::RationalResamplerBlock<complex_t>, 10, "Polyphase Resampler, Dec 100", 100.0, /**/ 1, 100);
}
