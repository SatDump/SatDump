#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include "modules/common/dsp/fir_filter.h"
#include "modules/common/dsp/clock_recovery_mm.h"
#include "modules/common/dsp/quadrature_demod.h"
#include <dsp/carrier_pll_psk.h>
#include <dsp/moving_average.h>
#include "modules/buffer.h"
#include <thread>
#include <fstream>
#include <dsp/random.h>

class FSKDemodModule : public ProcessingModule
{
protected:
    std::shared_ptr<libdsp::AgcCC> agc;
    std::shared_ptr<dsp::FIRFilterCCF> lpf;
    std::shared_ptr<dsp::QuadratureDemod> qua;
    std::shared_ptr<dsp::ClockRecoveryMMFF> rec;

    const int d_samplerate;
    const int d_symbolrate;
    const int d_buffer_size;
    const int d_lpf_cutoff;
    const int d_lpf_transition_width;

    int8_t *sym_buffer;

    // All FIFOs we use along the way
    dsp::stream<std::complex<float>> in_pipe;
    dsp::stream<std::complex<float>> agc_pipe;
    dsp::stream<std::complex<float>> lpf_pipe;
    dsp::stream<float> qua_pipe;
    dsp::stream<float> rec_pipe;

    std::atomic<bool> agcRun, lpfRun, quaRun, recRun;

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

    void fileThreadFunction();
    void agcThreadFunction();
    void lpfThreadFunction();
    void quaThreadFunction();
    void clockrecoveryThreadFunction();

    std::thread fileThread, agcThread, lpfThread, quaThread, recThread;

    std::ifstream data_in;
    std::ofstream data_out;

    std::atomic<size_t> filesize;
    std::atomic<size_t> progress;

    // UI Stuff
    libdsp::Random rng;

public:
    FSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    ~FSKDemodModule();
    void process();
    void drawUI();
    std::vector<ModuleDataType> getInputTypes();
    std::vector<ModuleDataType> getOutputTypes();

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};