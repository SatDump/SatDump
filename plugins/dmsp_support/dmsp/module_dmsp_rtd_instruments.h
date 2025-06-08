#pragma once

#include "instruments/ols/ols_rtd_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace dmsp
{
    class DMSPInstrumentsModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t rtd_frame[19];

        // Line CNT
        int ols_line_count = 0;

        // Readers
        ols::OLSRTDReader ols_reader;

        // Statuses
        instrument_status_t ols_status = DECODING;

    public:
        DMSPInstrumentsModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~DMSPInstrumentsModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace dmsp
