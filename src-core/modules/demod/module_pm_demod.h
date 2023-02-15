#pragma once

#include "module_demod_base.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "common/dsp/demod/pm_to_bpsk.h"

namespace demod
{
    class PMDemodModule : public BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::PLLCarrierTrackingBlock> pll;
        std::shared_ptr<dsp::PMToBPSK> pm_psk;
        std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
        std::shared_ptr<dsp::CostasLoopBlock> costas;
        std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;

        bool d_resample_after_pll = false;

        float d_pll_bw;
        float d_pll_max_offset = 0.5;
        float d_rrc_alpha;
        int d_rrc_taps = 31;
        float d_loop_bw = 0.005;

        float d_clock_gain_omega = pow(0.01, 2) / 4.0;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = 0.01;
        float d_clock_omega_relative_limit = 0.005f;

        uint64_t d_subccarier_offset = 0;

        int8_t *sym_buffer;

    public:
        PMDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~PMDemodModule();
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