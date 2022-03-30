#pragma once

#include "module_demod_base.h"
#include "common/dsp/random.h"
#include "common/dsp/fir.h"
#include "common/dsp/quadrature_demod.h"
#include "common/dsp/clock_recovery_mm.h"

namespace demod
{
    class FSKDemodModule : public BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::CCFIRBlock> lpf;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
        std::shared_ptr<dsp::FFMMClockRecoveryBlock> rec;

        int d_lpf_cutoff;
        int d_lpf_transition_width;

        int8_t *sym_buffer;

    public:
        FSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~FSKDemodModule();
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