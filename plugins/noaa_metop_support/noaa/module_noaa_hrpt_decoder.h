#pragma once

#include "common/widgets/constellation.h"
#include "noaa_deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include <fstream>

namespace noaa
{
    class NOAAHRPTDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::shared_ptr<NOAADeframer> def;

        int8_t *soft_buffer;

        int frame_count = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        NOAAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAAHRPTDecoderModule();
        void process();
        void drawUI(bool window);

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace noaa
