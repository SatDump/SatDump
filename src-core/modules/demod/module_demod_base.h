#pragma once

#include "core/module.h"
#include <thread>
#include <fstream>
#include <atomic>
#include "common/dsp/utils/agc.h"
#include "common/dsp/io/file_source.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "common/dsp/utils/correct_iq.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp/utils/freq_shift.h"
#include "common/dsp/utils/snr_estimator.h"
#include "common/widgets/constellation.h"
#include "common/widgets/snr_plot.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"
#include "common/dsp/utils/doppler_correct.h"
#include "common/dsp/io/file_sink.h"

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
        std::shared_ptr<dsp::FreqShiftBlock> freq_shift;
        std::shared_ptr<dsp::DopplerCorrectBlock> doppler_shift;
        std::shared_ptr<dsp::SplitterBlock> fft_splitter;
        std::shared_ptr<dsp::FFTPanBlock> fft_proc;
        std::shared_ptr<dsp::CorrectIQBlock<complex_t>> dc_blocker;
        std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> resampler;
        std::shared_ptr<dsp::AGCBlock<complex_t>> agc;

        std::string name = "Demodulator";

        // Defaults can be overwritten by an implementation's constructor
        int d_buffer_size = 8192;
        bool d_iq_swap = false;
        float d_agc_rate = 1e-2;
        bool d_dc_block = false;
        long d_frequency_shift = 0;
        long d_samplerate = -1;
        int d_symbolrate = 0;

        bool d_doppler_enable = false;
        float d_doppler_alpha = 0.01;

        std::string d_dump_intermediate = "";
        std::shared_ptr<dsp::FileSinkBlock> intermediate_file_sink;

        // Computed values
        float final_samplerate;
        float final_sps;

        // Min/Max sample per symbol the demodulator will accept before resampling the input
        float MIN_SPS = 1.1;
        float MAX_SPS = 4.0;
        bool resample = false;

        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        M2M4SNREstimator snr_estimator;
        float snr, peak_snr, signal, noise;

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

        // Stopping, etc
        bool demod_should_stop = false;
        bool demod_should_run()
        {
            return (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load()) && !demod_should_stop;
        }
        void drawStopButton();

    public:
        BaseDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~BaseDemodModule();
        virtual void initb(bool resample_here = true);
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