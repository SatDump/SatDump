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

        public:
            MetOpIASIDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace iasi
} // namespace metop