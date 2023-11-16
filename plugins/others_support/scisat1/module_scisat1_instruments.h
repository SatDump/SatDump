#pragma once

#include "core/module.h"
#include "instruments/maestro/maestro_reader.h"
#include "instruments/fts/fts_reader.h"

namespace scisat1
{
    namespace instruments
    {
        class SciSat1InstrumentsDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            fts::FTSReader fts_reader;
            maestro::MaestroReader maestro_reader;

            // Statuses
            instrument_status_t fts_status = DECODING;
            instrument_status_t maestro_status = DECODING;

        public:
            SciSat1InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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