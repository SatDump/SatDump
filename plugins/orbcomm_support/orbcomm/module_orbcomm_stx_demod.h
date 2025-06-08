#pragma once

#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/utils/correct_iq.h"
#include "pipeline/modules/demod/module_demod_base.h"
#include "stx_deframer.h"

#define ORBCOMM_STX_FRM_SIZE 4800

namespace orbcomm
{
    class OrbcommSTXDemodModule : public satdump::pipeline::demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
        std::shared_ptr<dsp::CorrectIQBlock<float>> iqc;
        std::shared_ptr<dsp::FIRBlock<float>> rrc;
        std::shared_ptr<dsp::MMClockRecoveryBlock<float>> rec;

        STXDeframer stx_deframer;

    public:
        OrbcommSTXDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~OrbcommSTXDemodModule();
        void init();
        void stop();
        void process();

        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace orbcomm