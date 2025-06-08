#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace bluewalker3
{
    class BW3DecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        const int d_cadu_size;
        const int d_payload_size;

    public:
        BW3DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace bluewalker3