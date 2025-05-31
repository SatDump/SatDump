#pragma once

#include "instruments/atlid/atlid_reader.h"
#include "instruments/msi/msi_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace earthcare
{
    namespace instruments
    {
        class EarthCAREInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            // Readers
            msi::MSIReader msi_reader;
            atlid::ATLIDReader atlid_reader;

            // Statuses
            instrument_status_t msi_status = DECODING;
            instrument_status_t atlid_status = DECODING;

        public:
            EarthCAREInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace earthcare