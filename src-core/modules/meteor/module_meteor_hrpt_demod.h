#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include "modules/common/dsp/fir_filter.h"
#include "modules/common/dsp/clock_recovery_mm.h"
#include <dsp/carrier_pll_psk.h>
#include <dsp/moving_average.h>
#include "modules/buffer.h"
#include <thread>
#include <fstream>
#include "deframer.h"
#include <dsp/random.h>

namespace meteor
{
    class METEORHRPTDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<libdsp::AgcCC> agc;
        std::shared_ptr<dsp::FIRFilterCCF> rrc;
        std::shared_ptr<libdsp::BPSKCarrierPLL> pll;
        std::shared_ptr<libdsp::MovingAverageFF> mov;
        std::shared_ptr<dsp::ClockRecoveryMMFF> rec;

        const int d_samplerate;
        const int d_buffer_size;

        uint8_t *bits_buffer;

        // All FIFOs we use along the way
       dsp::stream<std::complex<float>> in_pipe;
       dsp::stream<std::complex<float>> agc_pipe;
       dsp::stream<std::complex<float>> rrc_pipe;
       dsp::stream<float> pll_pipe;
       dsp::stream<float> mov_pipe;
       dsp::stream<float> rec_pipe;

        std::atomic<bool> agcRun, rrcRun, pllRun, movRun, recRun;

        // Int16 buffer
        int16_t *buffer_i16;
        // Int8 buffer
        int8_t *buffer_i8;
        // Uint8 buffer
        uint8_t *buffer_u8;

        bool f32 = false, i16 = false, i8 = false, w8 = false;

        void fileThreadFunction();
        void agcThreadFunction();
        void rrcThreadFunction();
        void pllThreadFunction();
        void movThreadFunction();
        void clockrecoveryThreadFunction();

        std::thread fileThread, agcThread, rrcThread, pllThread, recThread, movThread;

        std::ifstream data_in;
        std::ofstream data_out;

        uint8_t byteToWrite;
        int inByteToWrite = 0;

        std::vector<uint8_t> getBytes(uint8_t *bits, int length);

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // UI Stuff
        libdsp::Random rng;

    public:
        METEORHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~METEORHRPTDemodModule();
        void process();
        void drawUI();
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace meteor