#pragma once

#include "instruments/cips/cips_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace aim
{
    namespace instruments
    {
        class AIMInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            // Readers
            cips::CIPSReader cips_readers[4];

            // Statuses
            instrument_status_t cips_status[4] = {DECODING, DECODING, DECODING, DECODING};

        public:
            AIMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace aim