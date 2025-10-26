#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

#include <stdio.h>

namespace transit
{
    class TransitDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t last_frame[32]; 
        uint32_t last_fc = 0;
        FILE* debug_f;

    public:
        TransitDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; }
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace transit
