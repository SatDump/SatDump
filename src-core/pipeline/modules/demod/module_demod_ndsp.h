#pragma once

#include "dsp/displays/const_disp.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/hier/psk_demod.h"
#include "dsp/legacy/old2new.h"
#include "pipeline/module.h"
#include <complex.h>
#include <memory>

namespace satdump
{
    namespace pipeline
    {
        namespace demod
        {
            class NdspDemodModule : public ProcessingModule
            {
            protected:
                std::shared_ptr<ndsp::Old2NewBlock<complex_t>> old2new_block;
                std::shared_ptr<ndsp::PSKDemodHierBlock> demod_block;
                std::shared_ptr<ndsp::ConstellationDisplayBlock> const_block;

            public:
                NdspDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~NdspDemodModule();
                void init();
                void stop();
                void process();
                void drawUI(bool window);
                std::vector<ModuleDataType> getInputTypes();
                std::vector<ModuleDataType> getOutputTypes();

            public:
                static std::string getID() { return "ndsp_demod"; }
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams() { return {}; } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace demod
    } // namespace pipeline
} // namespace satdump