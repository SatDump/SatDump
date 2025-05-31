#pragma once

#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace geoscan
{
    class GEOSCANDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t *frame_buffer;

    public:
        GEOSCANDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GEOSCANDataDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace geoscan
