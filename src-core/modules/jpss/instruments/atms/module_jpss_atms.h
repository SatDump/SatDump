#pragma once

#include "module.h"

namespace jpss
{
    namespace atms
    {
        class JPSSATMSDecoderModule : public ProcessingModule
        {
        protected:
            bool npp_mode;
            int cadu_size;
            int mpdu_size;

        public:
            JPSSATMSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace atms
} // namespace jpss