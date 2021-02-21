#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include <dsp/fir_filter.h>
#include <dsp/carrier_pll_psk.h>
#include <dsp/clock_recovery_mm.h>
#include "noaa_deframer.h"
#include <dsp/pipe.h>
#include <thread>
#include <fstream>
#include <dsp/random.h>

namespace noaa
{
    class NOAAHRPTDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<libdsp::AgcCC> agc;
        std::shared_ptr<libdsp::BPSKCarrierPLL> pll;
        std::shared_ptr<libdsp::FIRFilterFFF> rrc;
        std::shared_ptr<libdsp::ClockRecoveryMMFF> rec;
        std::shared_ptr<NOAADeframer> def;

        const int d_samplerate;
        const int d_buffer_size;

        std::complex<float> *in_buffer, *in_buffer2;
        std::complex<float> *agc_buffer, *agc_buffer2;
        float *pll_buffer, *pll_buffer2;
        float *rrc_buffer, *rrc_buffer2;
        float *rec_buffer, *rec_buffer2;
        uint8_t *bits_buffer;

        // All FIFOs we use along the way
        libdsp::Pipe<std::complex<float>> *in_pipe;
        libdsp::Pipe<std::complex<float>> *agc_pipe;
        libdsp::Pipe<float> *rrc_pipe;
        libdsp::Pipe<float> *pll_pipe;
        libdsp::Pipe<float> *rec_pipe;

        std::atomic<bool> agcRun, rrcRun, pllRun, recRun;

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
        void clockrecoveryThreadFunction();

        std::thread fileThread, agcThread, rrcThread, pllThread, recThread;

        std::ifstream data_in;
        std::ofstream data_out;

        uint8_t byteToWrite;
        int inByteToWrite = 0;

        std::vector<uint8_t> getBytes(uint8_t *bits, int length);

        void volk_32f_binary_slicer_8i_generic(int8_t *cVector, const float *aVector, unsigned int num_points);

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        int frame_count = 0;

        // UI Stuff
        libdsp::Random rng;

    public:
        NOAAHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~NOAAHRPTDemodModule();
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
} // namespace noaa