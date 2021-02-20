#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include <dsp/fir_filter.h>
#include <dsp/carrier_pll_psk.h>
#include <dsp/clock_recovery_mm.h>
#include <dsp/moving_average.h>
#include <dsp/pipe.h>
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
        std::shared_ptr<libdsp::FIRFilterCCF> rrc;
        std::shared_ptr<libdsp::BPSKCarrierPLL> pll;
        std::shared_ptr<libdsp::MovingAverageFF> mov;
        std::shared_ptr<libdsp::ClockRecoveryMMFF> rec;

        const int d_samplerate;
        const int d_buffer_size;

        std::complex<float> *in_buffer, *in_buffer2;
        std::complex<float> *agc_buffer, *agc_buffer2;
        std::complex<float> *rrc_buffer, *rrc_buffer2;
        float *pll_buffer, *pll_buffer2;
        float *mov_buffer, *mov_buffer2;
        float *rec_buffer, *rec_buffer2;
        uint8_t *bits_buffer;

        // All FIFOs we use along the way
        libdsp::Pipe<std::complex<float>> *in_pipe;
        libdsp::Pipe<std::complex<float>> *agc_pipe;
        libdsp::Pipe<std::complex<float>> *rrc_pipe;
        libdsp::Pipe<float> *pll_pipe;
        libdsp::Pipe<float> *mov_pipe;
        libdsp::Pipe<float> *rec_pipe;

        bool agcRun, rrcRun, pllRun, movRun, recRun;

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

        void volk_32f_binary_slicer_8i_generic(int8_t *cVector, const float *aVector, unsigned int num_points);

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // UI Stuff
        libdsp::Random rng;

    public:
        METEORHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~METEORHRPTDemodModule();
        void process();
        void drawUI();
        void getInputType();
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace meteor