#pragma once

#include "module.h"

namespace meteor
{
    namespace msumr
    {
        class METEORMSUMRDecoderModule : public ProcessingModule
        {
        protected:
        
        public:
            METEORMSUMRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace msumr
} // namespace meteor