#pragma once

#include "ominous/instruments/csr/csr_lossy_reader.h"
#include "ominous/instruments/csr/csr_reader.h"
#include "ominous/instruments/loris/loris_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace ominous
{
    namespace instruments
    {
        class OminousInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            // Readers
            csr::CSRReader csr_reader;
            csr::CSRLossyReader csr_lossy_reader;
            loris::LORISReader loris_reader;

            // Statuses
            instrument_status_t csr_status = DECODING;
            instrument_status_t csr_lossy_status = DECODING;
            instrument_status_t loris_status = DECODING;

        public:
            OminousInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace ominous