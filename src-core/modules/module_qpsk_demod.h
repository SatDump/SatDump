#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include "modules/common/dsp/fir_filter.h"
#include "modules/common/dsp/clock_recovery_mm.h"
#include <dsp/costas_loop.h>
#include "buffer.h"
#include <thread>
#include <fstream>
#include <atomic>

#include "modules/common/dsp/new/agc.h"
#include "modules/common/dsp/new/fir.h"
#include "modules/common/dsp/new/costas_loop.h"
#include "modules/common/dsp/new/clock_recovery_mm.h"
#include "modules/common/dsp/new/file_source.h"

class QPSKDemodModule : public ProcessingModule
{
protected:
    std::shared_ptr<dsp::FileSourceBlock> file_source;
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

    // Int16 buffer
    int16_t *buffer_i16;
    // Int8 buffer
    int8_t *buffer_i8;
    // Uint8 buffer
    uint8_t *buffer_u8;

    bool f32 = false, i16 = false, i8 = false, w8 = false;

    int8_t clamp(float x)
    {
        if (x < -128.0)
            return -127;
        if (x > 127.0)
            return 127;
        return x;
    }

    std::ofstream data_out;

    std::atomic<size_t> filesize;
    std::atomic<size_t> progress;

public:
    QPSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    ~QPSKDemodModule();
    void process();
    void drawUI();
    std::vector<ModuleDataType> getInputTypes();
    std::vector<ModuleDataType> getOutputTypes();

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};