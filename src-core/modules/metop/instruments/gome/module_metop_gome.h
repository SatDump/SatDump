#pragma once

#include "module.h"

namespace metop
{
    namespace gome
    {
        class MetOpGOMEDecoderModule : public ProcessingModule
        {
        protected:
            bool write_all;

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            MetOpGOMEDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace gome
} // namespace metop