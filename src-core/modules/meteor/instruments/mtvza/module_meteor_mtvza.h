#pragma once

#include "module.h"

namespace meteor
{
    namespace mtvza
    {
        class METEORMTVZADecoderModule : public ProcessingModule
        {
        protected:
        
        public:
            METEORMTVZADecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace mtvza
} // namespace meteor