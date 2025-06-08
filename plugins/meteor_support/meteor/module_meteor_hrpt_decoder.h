#pragma once

#include "common/widgets/constellation.h"
#include "deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace meteor
{
    using namespace satdump::pipeline; // TODOREWORK

    class METEORHRPTDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::shared_ptr<CADUDeframer> def;

        int frame_count = 0;
        int8_t *soft_buffer;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~METEORHRPTDecoderModule();
        void process();
        void drawUI(bool window);

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace meteor