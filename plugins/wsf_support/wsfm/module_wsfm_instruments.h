#pragma once

#include "instruments/mwi/mwi_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace wsfm
{
    class WSFMInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        // Readers
        mwi::MWIReader mwi_reader;

        // Statuses
        instrument_status_t mwi_status = DECODING;

    public:
        WSFMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace wsfm