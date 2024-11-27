#pragma once

#include "modules/demod/module_demod_base.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/demod/delay_one_imag.h"

#include <complex>
#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/codings/viterbi/viterbi_1_2.h"
#include "common/codings/viterbi/viterbi27.h"
#include <fstream>
#include "common/codings/reedsolomon/reedsolomon.h"

namespace meteor
{
    class MeteorConcatLrptDemod : public demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
        std::shared_ptr<dsp::CostasLoopBlock> pll;
        std::shared_ptr<dsp::DelayOneImagBlock> delay;
        std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;

        float d_rrc_alpha;
        int d_rrc_taps = 31;
        float d_loop_bw;

        float d_clock_gain_omega = pow(8.7e-3, 2) / 4.0;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = 8.7e-3;
        float d_clock_omega_relative_limit = 0.005f;

        const bool m2x_mode;
        const bool diff_decode;
        const bool interleaved;
        const int deframer_outsync_after;

        int8_t *sym_buffer;
        int8_t *_buffer, *_buffer2;
        int8_t *buffer, *buffer2;

        std::thread deframer_thread;
        std::shared_ptr<dsp::RingBuffer<int8_t>> thread_fifo;
        std::ofstream data_out;

        int viterbi_lock;
        float viterbi_ber;

        bool locked = false;
        int errors[4];
        int cor;

        std::shared_ptr<viterbi::Viterbi27> viterbi;

        std::shared_ptr<viterbi::Viterbi1_2> viterbin, viterbin2;
        std::shared_ptr<deframing::BPSK_CCSDS_Deframer> deframer;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

    public:
        MeteorConcatLrptDemod(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~MeteorConcatLrptDemod();
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
}