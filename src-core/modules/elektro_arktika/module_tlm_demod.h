#pragma once

#include "module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/dsp/agc.h"
#include "common/dsp/fir.h"
#include "common/dsp/costas_loop.h"
#include "common/dsp/clock_recovery_mm.h"
#include "common/dsp/file_source.h"
#include "common/dsp/dc_blocker.h"
#include "common/dsp/rational_resampler.h"
#include "common/snr_estimator.h"
#include "common/dsp/pll_carrier_tracking.h"
#include "common/dsp/freq_shift.h"
#include "common/repack_bits_byte.h"
#include "tlm_deframer.h"
#include "common/widgets/constellation.h"
#include "common/widgets/snr_plot.h"

namespace elektro_arktika
{
    class TLMDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<dsp::FileSourceBlock> file_source;
        std::shared_ptr<dsp::CCRationalResamplerBlock> res;
        std::shared_ptr<dsp::AGCBlock> agc;
        std::shared_ptr<dsp::PLLCarrierTrackingBlock> cpl;
        std::shared_ptr<dsp::DCBlockerBlock> dcb;
        std::shared_ptr<dsp::FreqShiftBlock> shi;
        std::shared_ptr<dsp::CCFIRBlock> rrc;
        std::shared_ptr<dsp::CostasLoopBlock> pll;
        std::shared_ptr<dsp::CCMMClockRecoveryBlock> rec;

        const int d_samplerate;
        /*const*/ int d_buffer_size;

        RepackBitsByte repacker;
        uint8_t *bits_buffer;
        float *real_buffer;
        uint8_t *repacked_buffer;

        const int MAX_SPS = 10; // Maximum sample per symbol the demodulator will accept before resampling the input
        bool resample = false;

        int8_t *sym_buffer;

        int8_t clamp(float x)
        {
            if (x < -128.0)
                return -127;
            if (x > 127.0)
                return 127;
            return x;
        }

        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        M2M4SNREstimator snr_estimator;
        float snr, peak_snr;

        CADUDeframer deframer;

        // UI Stuff
        widgets::ConstellationViewer constellation;
        widgets::SNRPlotViewer snr_plot;

    public:
        TLMDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~TLMDemodModule();
        void init();
        void stop();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
}