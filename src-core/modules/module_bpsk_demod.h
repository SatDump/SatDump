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
#include "common/dsp/snr_estimator.h"
#include "common/widgets/constellation.h"
#include "common/widgets/snr_plot.h"
#include "common/dsp/correct_iq.h"

class BPSKDemodModule : public ProcessingModule
{
protected:
    std::shared_ptr<dsp::FileSourceBlock> file_source;
    std::shared_ptr<dsp::DCBlockerBlock> dcb;
    std::shared_ptr<dsp::CCRationalResamplerBlock> res;
    std::shared_ptr<dsp::AGCBlock> agc;
    std::shared_ptr<dsp::CCFIRBlock> rrc;
    std::shared_ptr<dsp::CostasLoopBlock> pll;
    std::shared_ptr<dsp::CorrectIQBlock> cor;
    std::shared_ptr<dsp::CCMMClockRecoveryBlock> rec;

    const float d_agc_rate;
    const int d_samplerate;
    const int d_symbolrate;
    const float d_rrc_alpha;
    const int d_rrc_taps;
    const float d_loop_bw;
    /*const*/ int d_buffer_size;
    const bool d_dc_block;
    const bool d_post_costas_dc;

    const int MAX_SPS = 3; // Maximum sample per symbol the demodulator will accept before resampling the input
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

    // UI Stuff
    widgets::ConstellationViewer constellation;
    widgets::SNRPlotViewer snr_plot;

public:
    BPSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    ~BPSKDemodModule();
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
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
};
