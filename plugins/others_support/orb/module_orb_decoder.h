#pragma once

#include "image_parser.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace orb
{
    class ORBDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        ImageParser l2_parser;
        ImageParser l3_parser;

    public:
        ORBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace orb
