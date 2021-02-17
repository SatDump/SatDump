#pragma once

#include "module.h"

namespace proba
{
    namespace hrc
    {
        class ProbaHRCDecoderModule : public ProcessingModule
        {
        protected:
        public:
            ProbaHRCDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace hrc
} // namespace proba