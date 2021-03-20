#pragma once

#include "module.h"

namespace eos
{
    namespace modis
    {
        class EOSMODISDecoderModule : public ProcessingModule
        {
        protected:
            bool terra;
            bool bowtie;

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            EOSMODISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace modis
} // namespace eos