#pragma once

#include "instruments/fts/fts_reader.h"
#include "instruments/maestro/maestro_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace scisat1
{
    namespace instruments
    {
        class SciSat1InstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
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
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace scisat1