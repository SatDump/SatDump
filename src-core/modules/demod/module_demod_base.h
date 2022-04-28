#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include <atomic>
#include "common/dsp/agc.h"
#include "common/dsp/file_source.h"
#include "common/dsp/splitter.h"
#include "common/dsp/fft.h"
#include "common/dsp/correct_iq.h"
#include "common/dsp/rational_resampler.h"
#include "common/dsp/snr_estimator.h"
#include "common/widgets/constellation.h"
#include "common/widgets/snr_plot.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

namespace demod
{
    /*
    This class is meant to handle all common basics shared between demodulators,
    such as file input, DC Blocking, resampling if the symbolrate is out of bounds
    and so on.

    This is not meant to be used by itself, and would serve no purpose anyway
    */
    class BaseDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<dsp::FileSourceBlock> file_source;
        std::shared_ptr<dsp::SplitterBlock> fft_splitter;
        std::shared_ptr<dsp::FFTBlock> fft_proc;
        std::shared_ptr<dsp::CorrectIQBlock> dc_blocker;
        std::shared_ptr<dsp::CCRationalResamplerBlock> resampler;
        std::shared_ptr<dsp::AGCBlock> agc;

        std::string name = "Demodulator";

        // Defaults can be overwritten by an implementation's constructor
        int d_buffer_size = 8192;
        bool d_iq_swap = false;
        float d_agc_rate = 1e-2;
        bool d_dc_block = false;
        int d_samplerate = -1;
        int d_symbolrate = 0;

        // Computed values
        float final_samplerate;
        float final_sps;

        // Min/Max sample per symbol the demodulator will accept before resampling the input
        float MIN_SPS = 1.1;
        float MAX_SPS = 3.0;
        bool resample = false;

        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        M2M4SNREstimator snr_estimator;
        float snr, peak_snr;

        bool show_freq = false;
        float display_freq = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;
        widgets::SNRPlotViewer snr_plot;
        bool show_fft = false;
        std::shared_ptr<widgets::FFTPlot> fft_plot;
        std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

        bool showWaterfall = false;
        void drawFFT();

        // Util
        int8_t clamp(float x)
        {
            if (x < -128.0)
                return -127;
            if (x > 127.0)
                return 127;
            return x;
        }

    public:
        BaseDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~BaseDemodModule();
        virtual void init();
        virtual void start();
        virtual void stop();
        // void process();
        virtual void drawUI(bool window);
        virtual std::vector<ModuleDataType> getInputTypes();
        virtual std::vector<ModuleDataType> getOutputTypes();

    public:
        // static std::string getID();
        // virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        // static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}