#pragma once

#include "common/dsp/clock_recovery/clock_recovery_gardner.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/utils/agc2.h"
#include "common/dsp/utils/random.h"
#include "module_demod_base.h"

namespace satdump
{
    namespace pipeline
    {
        namespace demod
        {
            class XFSKBurstDemodModule : public BaseDemodModule
            {
            protected:
                std::shared_ptr<dsp::FIRBlock<complex_t>> lpf1;
                std::shared_ptr<dsp::QuadratureDemodBlock> qua;
                std::shared_ptr<dsp::AGC2Block<float>> agc2;
                std::shared_ptr<dsp::FIRBlock<float>> lpf2;
                std::shared_ptr<dsp::SmartResamplerBlock<float>> resamplerf;
                // std::shared_ptr<dsp::GardnerClockRecovery2Block> rec;
                std::shared_ptr<dsp::GardnerClockRecoveryBlock<float>> rec;

                const float d_deviation;

                bool d_resample_after_quad = false;

                int8_t *sym_buffer;

            public:
                XFSKBurstDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~XFSKBurstDemodModule();
                void init();
                void stop();
                void process();

                nlohmann::json getModuleStats();

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams() { return {}; } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace demod
    } // namespace pipeline
} // namespace satdump
