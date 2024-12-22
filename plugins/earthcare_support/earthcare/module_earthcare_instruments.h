#pragma once

#include "core/module.h"

#include "instruments/msi/msi_reader.h"

namespace earthcare
{
    namespace instruments
    {
        class EarthCAREInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            msi::MSIReader msi_reader;

            // Statuses
            instrument_status_t msi_status = DECODING;

        public:
            EarthCAREInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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