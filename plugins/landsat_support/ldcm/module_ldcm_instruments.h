#pragma once

#include "core/module.h"

#include "instruments/tirs/tirs_reader.h"

namespace ldcm
{
    namespace instruments
    {
        class LDCMInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            tirs::TIRSReader tirs_reader1;
            tirs::TIRSReader tirs_reader2;
            tirs::TIRSReader tirs_reader3;

            // Statuses
            instrument_status_t tirs_status = DECODING;

        public:
            LDCMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace amsu
} // namespace metop