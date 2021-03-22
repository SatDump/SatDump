#pragma once

#include "module.h"
#include <complex>
#include <thread>
#include <fstream>
#include <atomic>
#include "modules/common/dsp/agc.h"
#include "modules/common/dsp/fir.h"
#include "modules/common/dsp/costas_loop.h"
#include "modules/common/dsp/clock_recovery_mm.h"
#include "modules/common/dsp/file_source.h"
#include "modules/common/dsp/dc_blocker.h"

class QPSKDemodModule : public ProcessingModule
{
protected:
    std::shared_ptr<dsp::FileSourceBlock> file_source;
    std::shared_ptr<dsp::DCBlockerBlock> dcb;
    std::shared_ptr<dsp::AGCBlock> agc;
    std::shared_ptr<dsp::CCFIRBlock> rrc;
    std::shared_ptr<dsp::CostasLoopBlock> pll;
    std::shared_ptr<dsp::CCMMClockRecoveryBlock> rec;

    const float d_agc_rate;
    const int d_samplerate;
    const int d_symbolrate;
    const float d_rrc_alpha;
    const int d_rrc_taps;
    const float d_loop_bw;
    const int d_buffer_size;
    const bool d_dc_block;

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

public:
    QPSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    ~QPSKDemodModule();
    void init();
    void process();
    void drawUI();
    std::vector<ModuleDataType> getInputTypes();
    std::vector<ModuleDataType> getOutputTypes();

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};