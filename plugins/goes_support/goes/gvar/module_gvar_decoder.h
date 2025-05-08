#pragma once

#include "common/dsp/utils/random.h"
#include "core/module.h"
#include <complex>
#include <fstream>

namespace goes
{
    namespace gvar
    {
        class GVARDecoderModule : public ProcessingModule
        {
        protected:
            // Read buffer
            int8_t *buffer;

            std::ifstream data_in;
            std::ofstream data_out;
            std::atomic<uint64_t> filesize = 0;
            std::atomic<uint64_t> progress = 0;

            // UI Stuff
            dsp::Random rng;

        public:
            GVARDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GVARDecoderModule();
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
    } // namespace gvar
} // namespace goes