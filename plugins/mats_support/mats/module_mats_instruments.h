#pragma once

#include "core/module.h"

#include "instruments/mats/mats_reader.h"

namespace mats
{
    namespace instruments
    {
        class MATSInstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            mats::MATSReader mats_reader;

            // Statuses
            instrument_status_t mats_status = DECODING;

        public:
            MATSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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