#pragma once

#include "module.h"

namespace goes
{
    namespace hrit
    {
        class GOESLRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace avhrr
} // namespace metop