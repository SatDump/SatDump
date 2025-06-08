#pragma once

#include "common/widgets/constellation.h"
#include "dmsp_deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace dmsp
{
    class DMSPRTDDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::shared_ptr<DMSP_Deframer> def;

        int8_t *soft_buffer;
        uint8_t *soft_bits;
        uint8_t *output_frames;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        DMSPRTDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~DMSPRTDDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace dmsp
