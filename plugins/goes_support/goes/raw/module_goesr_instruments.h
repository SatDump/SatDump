#pragma once

#include "instruments/suvi/suvi_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace goes
{
    namespace instruments
    {
        class GOESRInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            // Readers
            suvi::SUVIReader suvi_reader;

            // Statuses
            // instrument_status_t suvi_status = DECODING;

        public:
            GOESRInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace goes