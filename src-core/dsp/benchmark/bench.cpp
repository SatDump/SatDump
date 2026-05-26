#include "bench.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/complex.h"
#include "common/dsp/filter/fft_filter.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/utils/freq_shift.h"
#include "dsp/agc/agc.h"
#include "dsp/agc/agc_fast.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/clock_recovery/clock_recovery_mm_fast.h"
#include "dsp/clock_recovery/simple_gardner_recovery.h"
#include "dsp/clock_recovery/simple_zc_recovery.h"
#include "dsp/ddc/fft_ddc.h"
#include "dsp/filter/fft.h"
#include "dsp/filter/fir.h"
#include "dsp/path/splitter.h"
#include "dsp/pll/costas.h"
#include "dsp/pll/costas_fast.h"
#include "dsp/utils/freq_shift.h"
#include "helpers.h"
#include "logger.h"

namespace satdump
{
    namespace ndsp
    {
        std::vector<std::string> getBenchCategories()
        {
            return {
                "fft_ddc",              //
                "simple_gardner",       //
                "simple_zero_crossing", //
                "agc",                  //
                "costas",               //
                "rrc",                  //
                "mm_recovery",          //
                "splitter",             //
                "freq_shift",           //
                "resamplers",           //
            };
        }

        bool catReq(std::vector<std::string> &cats, std::string cat)
        {
            if (cats.size() == 0)
                return true;
            for (auto &c : cats)
                if (c == cat)
                    return true;
            return false;
        }

        std::vector<DSPBenchResult> runBenchmarks(std::vector<std::string> cats)
        {
            std::vector<DSPBenchResult> res;

            if (catReq(cats, "fft_ddc"))
            {
                logger->debug("Benchmarking FFT DDC...");
                for (int decim : {2, 8, 16, 32, 50, 64, 128, 256})
                {
                    auto p = std::make_shared<satdump::ndsp::FFTDDCBlock>();
                    p->set_cfg("frequency", 100e6);
                    p->set_cfg("samplerate", 6e6);
                    p->add_output("out1", 101e6, 200e3, decim);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "FFT DDC, 1 output, decim " + std::to_string(decim), decim);
                    res.push_back(r);
                }
            }

            if (catReq(cats, "simple_gardner"))
            {
                logger->debug("Benchmarking Simple Gardner Recovery...");
                for (float v : {1.2, 2.0, 3.0, 40.})
                {
                    auto p = std::make_shared<satdump::ndsp::SimpleGardnerRecoveryBlock>();
                    p->set_cfg("sps", v);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "Simple Gardner Recovery, omega " + std::to_string(v));
                    res.push_back(r);
                }
            }

            if (catReq(cats, "simple_zero_crossing"))
            {
                logger->debug("Benchmarking Simple Zero Crossing Recovery...");
                for (float v : {1.2, 2.0, 3.0, 40.})
                {
                    auto p = std::make_shared<satdump::ndsp::SimpleZeroCrossingRecoveryBlock>();
                    p->set_cfg("sps", v);
                    auto r = benchmarkNDSPBlock<float>(p, 10, "Simple Zero Crossing Recovery, omega " + std::to_string(v));
                    res.push_back(r);
                }
            }

            if (catReq(cats, "agc"))
            {
                logger->debug("Benchmarking AGC...");

                benchmarkLegacyBlock(complex_t, complex_t, dsp::AGCBlock<complex_t>, 10, "AGC, Default", 1.0, /**/ 0.01, 1.0, 1.0, 65535);

                {
                    auto p = std::make_shared<satdump::ndsp::AGCBlock<complex_t>>();
                    p->p_rate = 0.01;
                    p->p_reference = 1.0;
                    p->p_gain = 1.0;
                    p->p_max_gain = 65535;
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "AGC, New");
                    res.push_back(r);
                }
                {
                    auto p = std::make_shared<satdump::ndsp::AGCFastBlock<complex_t>>();
                    p->p_rate = 0.01;
                    p->p_reference = 1.0;
                    p->p_gain = 1.0;
                    p->p_max_gain = 65535;
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "AGC Fast, New");
                    res.push_back(r);
                }
            }

            if (catReq(cats, "costas"))
            {
                logger->debug("Benchmarking Costas Loop...");

                benchmarkLegacyBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 2", 1.0, /**/ 0.02, 2);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 4", 1.0, /**/ 0.02, 4);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::CostasLoopBlock, 10, "Costas Loop, Order 8", 1.0, /**/ 0.02, 8);

                for (int v : {2, 4, 8})
                {
                    auto p = std::make_shared<satdump::ndsp::CostasBlock>();
                    p->set_cfg("order", v);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "Costas Loop New, Order " + std::to_string(v));
                    res.push_back(r);
                }

                for (int v : {2, 4, 8})
                {
                    auto p = std::make_shared<satdump::ndsp::CostasFastBlock>();
                    p->set_cfg("order", v);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "Costas Loop Fast New, Order " + std::to_string(v));
                    res.push_back(r);
                }
            }

            if (catReq(cats, "rrc"))
            {
                logger->debug("Benchmarking FIR RRC...");

                benchmarkLegacyBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 31 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31));
                benchmarkLegacyBlock(complex_t, complex_t, dsp::FIRBlock<complex_t>, 10, "FIR RRC, 361 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361));

                benchmarkLegacyBlock(complex_t, complex_t, dsp::FFTFilterBlock<complex_t>, 10, "FFT RRC, 31 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31));
                benchmarkLegacyBlock(complex_t, complex_t, dsp::FFTFilterBlock<complex_t>, 10, "FFT RRC, 361 taps, omega 2.0", 1.0, /**/ dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361));

                {
                    auto p = std::make_shared<satdump::ndsp::FIRBlock<complex_t>>();
                    p->p_taps = dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "FIR New RRC, 31 taps, omega 2.0");
                    res.push_back(r);
                }
                {
                    auto p = std::make_shared<satdump::ndsp::FIRBlock<complex_t>>();
                    p->p_taps = dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "FIR New RRC, 361 taps, omega 2.0");
                    res.push_back(r);
                }

                {
                    auto p = std::make_shared<satdump::ndsp::FFTFilterBlock<complex_t>>();
                    p->p_taps = dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 31);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "FFT New RRC, 31 taps, omega 2.0");
                    res.push_back(r);
                }
                {
                    auto p = std::make_shared<satdump::ndsp::FFTFilterBlock<complex_t>>();
                    p->p_taps = dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.35, 361);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "FFT New RRC, 361 taps, omega 2.0");
                    res.push_back(r);
                }
            }

            if (catReq(cats, "mm_recovery"))
            {
                logger->debug("Benchmarking MM Recovery...");

                benchmarkLegacyBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 1.2", 1.0, /**/ 1.2, 0.1, 0.5, 0.01, 0.01);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 2.0", 1.0, /**/ 2.0, 0.1, 0.5, 0.01, 0.01);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::MMClockRecoveryBlock<complex_t>, 10, "MM Recovery, omega 3.0", 1.0, /**/ 3.0, 0.1, 0.5, 0.01, 0.01);

                for (float v : {1.2, 2.0, 3.0})
                {
                    auto p = std::make_shared<satdump::ndsp::MMClockRecoveryBlock<complex_t>>();
                    p->set_cfg("omega", v);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "MM NEW Recovery, omega " + std::to_string(v));
                    res.push_back(r);
                }

                for (float v : {1.2, 2.0, 3.0})
                {
                    auto p = std::make_shared<satdump::ndsp::MMClockRecoveryFastBlock<complex_t>>();
                    p->set_cfg("omega", v);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "MM NEW Recovery Fast, omega " + std::to_string(v));
                    res.push_back(r);
                }
            }

            if (catReq(cats, "splitter"))
            {
                logger->debug("Benchmarking Splitter (1 out)...");

                // benchmarkLegacyBlock(complex_t, complex_t, dsp::SplitterBlock, 10, "Freq Shift, Default", 1.0);

                {
                    auto p = std::make_shared<satdump::ndsp::SplitterBlock<complex_t>>();
                    p->set_cfg("noutputs", 1);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "Splitter, New");
                    res.push_back(r);
                }
            }

            if (catReq(cats, "freq_shift"))
            {
                logger->debug("Benchmarking Freq Shift...");

                benchmarkLegacyBlock(complex_t, complex_t, dsp::FreqShiftBlock, 10, "Freq Shift, Default", 1.0, /**/ 1e6, 100e3);

                {
                    auto p = std::make_shared<satdump::ndsp::FreqShiftBlock>();
                    p->set_cfg("samplerate", 1e6);
                    p->set_cfg("freq_shift", 100e3);
                    auto r = benchmarkNDSPBlock<complex_t>(p, 10, "Freq Shift, New");
                    res.push_back(r);
                }
            }

            if (catReq(cats, "resamplers"))
            {
                logger->debug("Benchmarking Resamplers...");

                benchmarkLegacyBlock(complex_t, complex_t, dsp::SmartResamplerBlock<complex_t>, 10, "Smart Resampler, Dec 10", 10.0, /**/ 1, 10);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::SmartResamplerBlock<complex_t>, 10, "Smart Resampler, Dec 100", 100.0, /**/ 1, 100);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::RationalResamplerBlock<complex_t>, 10, "Polyphase Resampler, Dec 10", 10.0, /**/ 1, 10);
                benchmarkLegacyBlock(complex_t, complex_t, dsp::RationalResamplerBlock<complex_t>, 10, "Polyphase Resampler, Dec 100", 100.0, /**/ 1, 100);
            }

            return res;
        }
    } // namespace ndsp
} // namespace satdump