#pragma once

#include "modules/demod/module_demod_base.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/costas_loop.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"

namespace terra
{
    class TerraDBDemodModule : public demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
        std::shared_ptr<dsp::CostasLoopBlock> pll;
        std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;

        int8_t *sym_buffer;

    public:
        TerraDBDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~TerraDBDemodModule();
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
