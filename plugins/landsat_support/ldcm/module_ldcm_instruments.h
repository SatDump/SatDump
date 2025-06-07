#pragma once

#include "instruments/tirs/tirs_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace ldcm
{
    namespace instruments
    {
        class LDCMInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
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
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace ldcm