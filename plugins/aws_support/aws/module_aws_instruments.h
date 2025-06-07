#pragma once

#include "instruments/mwr/mwr_reader.h"
#include "instruments/navatt/navatt_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace aws
{
    class AWSInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        // Readers
        mwr::MWRReader mwr_reader;
        mwr::MWRReader mwr_dump_reader;
        navatt::NavAttReader navatt_reader;

        // Statuses
        instrument_status_t mwr_status = DECODING;
        instrument_status_t mwr_dump_status = DECODING;

    public:
        AWSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace aws