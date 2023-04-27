#pragma once

#include "module_demod_base.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/clock_recovery/clock_recovery_gardner.h"
#include "common/dsp/demod/delay_one_imag.h"

namespace demod
{
    class PSKDemodModule : public BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
        std::shared_ptr<dsp::CostasLoopBlock> pll;
        std::shared_ptr<dsp::CorrectIQBlock<complex_t>> post_pll_dc;
        std::shared_ptr<dsp::DelayOneImagBlock> delay;
        std::shared_ptr<dsp::GardnerClockRecoveryBlock<complex_t>> rec;

        std::string constellation_type;
        bool is_bpsk, is_oqpsk;

        float d_rrc_alpha;
        int d_rrc_taps = 31;
        float d_loop_bw;
        bool d_post_costas_dc_blocking = false;

        float d_clock_gain_omega = pow(8.7e-3, 2) / 4.0;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = 8.7e-3;
        float d_clock_omega_relative_limit = 0.005f;

        int8_t *sym_buffer;

    public:
        PSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~PSKDemodModule();
        void init();
        void stop();
        void process();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}