#pragma once

#include "common/simple_deframer.h"
#include "dmsp/instruments/ssmis/ssmis_reader.h"
#include "dmsp/instruments/utils.h"
#include "instruments/ols/ols_rtd_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace dmsp
{
    class DMSPInstrumentsModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t rtd_frame[19];

        // Readers
        ols::OLSRTDReader ols_reader;
        ssmis::SSMISReader ssmis_reader;

        TERDATSDemuxer terdats;
        std::shared_ptr<def::SimpleDeframer> ssmis_deframer;

        // Statuses
        instrument_status_t ols_status = DECODING;
        instrument_status_t ssmis_status = DECODING;

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
