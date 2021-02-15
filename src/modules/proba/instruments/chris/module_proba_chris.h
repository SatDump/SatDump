#pragma once

#include "module.h"

namespace proba
{
    namespace chris
    {
        class ProbaCHRISDecoderModule : public ProcessingModule
        {
        protected:
        
        public:
            ProbaCHRISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace avhrr
} // namespace noaa