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

        public:
            EOSMODISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace modis
} // namespace eos