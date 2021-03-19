#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include <thread>
#include <fstream>
#include "modules/common/repack_bits_byte.h"
#include <dsp/random.h>
#include "modules/common/dsp/agc.h"
#include "modules/common/dsp/fir.h"
#include "modules/common/dsp/costas_loop.h"
#include "modules/common/dsp/clock_recovery_mm.h"
#include "modules/common/dsp/file_source.h"
#include "modules/common/dsp/bpsk_carrier_pll.h"
#include "dsb_deframer.h"

namespace noaa
{
    class NOAADSBDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<dsp::FileSourceBlock> file_source;
        std::shared_ptr<dsp::AGCBlock> agc;
        std::shared_ptr<dsp::BPSKCarrierPLLBlock> pll;
        std::shared_ptr<dsp::FFFIRBlock> rrc;
        std::shared_ptr<dsp::FFMMClockRecoveryBlock> rec;
        std::shared_ptr<RepackBitsByte> rep;
        std::shared_ptr<DSBDeframer> def;

        const int d_samplerate;
        const int d_buffer_size;

        uint8_t *bits_buffer;
        uint8_t *bytes_buffer;
        uint8_t *manchester_buffer;

        std::vector<uint8_t> defra_buf;

        std::ofstream data_out;

        int frame_count = 0;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI Stuff
        libdsp::Random rng;

    public:
        NOAADSBDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~NOAADSBDemodModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace noaa