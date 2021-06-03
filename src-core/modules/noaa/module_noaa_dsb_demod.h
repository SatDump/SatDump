#pragma once

#include "module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/repack_bits_byte.h"
#include "common/dsp/lib/random.h"
#include "common/dsp/agc.h"
#include "common/dsp/fir.h"
#include "common/dsp/costas_loop.h"
#include "common/dsp/clock_recovery_mm.h"
#include "common/dsp/file_source.h"
#include "common/dsp/bpsk_carrier_pll.h"
#include "common/dsp/rational_resampler.h"
#include "dsb_deframer.h"

namespace noaa
{
    class NOAADSBDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<dsp::FileSourceBlock> file_source;
        std::shared_ptr<dsp::CCRationalResamplerBlock> res;
        std::shared_ptr<dsp::AGCBlock> agc;
        std::shared_ptr<dsp::BPSKCarrierPLLBlock> pll;
        std::shared_ptr<dsp::FFFIRBlock> rrc;
        std::shared_ptr<dsp::FFMMClockRecoveryBlock> rec;
        std::shared_ptr<RepackBitsByte> rep;
        std::shared_ptr<DSBDeframer> def;

        const int d_samplerate;
        /*const*/ int d_buffer_size;

        const int MAX_SPS = 4; // Maximum sample per symbol the demodulator will accept before resampling the input
        bool resample = false;

        uint8_t *bits_buffer;
        uint8_t *bytes_buffer;
        uint8_t *manchester_buffer;

        std::vector<uint8_t> defra_buf;

        std::ofstream data_out;

        int frame_count = 0;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI Stuff
        dsp::Random rng;

    public:
        NOAADSBDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~NOAADSBDemodModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace noaa