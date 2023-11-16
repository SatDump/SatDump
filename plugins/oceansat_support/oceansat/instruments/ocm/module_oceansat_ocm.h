#pragma once

#include "core/module.h"
#include "ocm_reader.h"

namespace oceansat
{
    namespace ocm
    {
        class OceansatOCMDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            OCMReader ocm_reader;
            instrument_status_t ocm_status = DECODING;

        public:
            OceansatOCMDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace avhrr
} // namespace noaa