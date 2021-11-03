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

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            JPSSATMSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace atms
} // namespace jpss