#pragma once

#include "module.h"

namespace metop
{
    namespace iasi
    {
        class MetOpIASIDecoderModule : public ProcessingModule
        {
        protected:
            bool write_all;

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            MetOpIASIDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace iasi
} // namespace metop