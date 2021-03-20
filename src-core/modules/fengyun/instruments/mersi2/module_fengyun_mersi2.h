#pragma once

#include "module.h"

namespace fengyun
{
    namespace mersi2
    {
        class FengyunMERSI2DecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;
            bool bowtie;

        public:
            FengyunMERSI2DecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace virr
} // namespace fengyun