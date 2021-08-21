#pragma once

#include "module.h"

namespace jpss
{
    namespace cris
    {
        class JPSSCrISDecoderModule : public ProcessingModule
        {
        protected:
            bool npp_mode;
            int cadu_size;
            int mpdu_size;

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            JPSSCrISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace atms
} // namespace jpss