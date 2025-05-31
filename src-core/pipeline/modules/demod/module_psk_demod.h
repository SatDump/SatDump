#pragma once

#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/demod/delay_one_imag.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "module_demod_base.h"

namespace satdump
{
    namespace pipeline
    {
        namespace demod
        {
            class PSKDemodModule : public BaseDemodModule
            {
            protected:
                std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
                std::shared_ptr<dsp::PLLCarrierTrackingBlock> carrier_pll;
                std::shared_ptr<dsp::CorrectIQBlock<complex_t>> carrier_dc;
                std::shared_ptr<dsp::CostasLoopBlock> pll;
                std::shared_ptr<dsp::CorrectIQBlock<complex_t>> post_pll_dc;
                std::shared_ptr<dsp::DelayOneImagBlock> delay;
                std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;

                std::string constellation_type;
                bool is_bpsk, is_oqpsk;

                float d_rrc_alpha;
                int d_rrc_taps = 31;
                float d_loop_bw;
                bool d_post_costas_dc_blocking = false;
                bool d_has_carrier = false;

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

                nlohmann::json getModuleStats();

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams()
                {
                    nlohmann::json v = BaseDemodModule::getParams();
                    v["constellation"] = "bpsk";
                    v["rrc_alpha"] = 0.35;
                    v["rrc_taps"] = 31;
                    v["pll_bw"] = 0.005;
                    v["post_costas_dc"] = false;
                    v["has_carrier"] = false;
                    v["clock_alpha"] = 0;
                    v["clock_gain_omega"] = 0;
                    v["clock_mu"] = 0;
                    v["clock_gain_mu"] = 0;
                    v["clock_omega_relative_limit"] = 0;
                    v["carrier_pll_bw"] = 0;
                    v["carrier_pll_max_offset"] = 3.14;
                    v["costas_max_offset"] = 1.0;
                    return v;
                } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace demod
    } // namespace pipeline
} // namespace satdump