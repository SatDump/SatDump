#pragma once

#include "codings/dvbs_ts_deframer.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/demod/delay_one_imag.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/costas_loop.h"
#include "dvbs_defra.h"
#include "dvbs_syms_to_soft.h"
#include "dvbs_vit.h"
#include "pipeline/modules/demod/module_demod_base.h"
#include "viterbi_all.h"

namespace satdump
{
    namespace pipeline
    {
        namespace dvb
        {
            class DVBSDemodModule : public demod::BaseDemodModule
            {
            protected:
                std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
                std::shared_ptr<dsp::CostasLoopBlock> pll;
                std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;
                std::shared_ptr<dvbs::DVBSymToSoftBlock> sts;
                std::shared_ptr<dvbs::DVBSVitBlock> vit;
                std::shared_ptr<dvbs::DVBSDefra> def;

                float d_rrc_alpha = 0.35;
                int d_rrc_taps = 31;
                float d_loop_bw = 0.003;

                float d_clock_gain_omega = pow(8.7e-3, 2) / 4.0;
                float d_clock_mu = 0.5f;
                float d_clock_gain_mu = 8.7e-3;
                float d_clock_omega_relative_limit = 0.005f;

                viterbi::Viterbi_DVBS viterbi;

                deframing::DVBS_TS_Deframer ts_deframer;

                int errors[8];
                std::string rate = "";

                // UI Stuff
                float ber_history[200];

            public:
                DVBSDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~DVBSDemodModule();
                void init();
                void stop();
                void process();
                void drawUI(bool window);

                nlohmann::json getModuleStats();

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams() { return {}; } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace dvb
    } // namespace pipeline
} // namespace satdump