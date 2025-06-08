#pragma once

#include "common/repack_bits_byte.h"
#include "common/widgets/constellation.h"
#include "dsb_deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace noaa
{
    class NOAADSBDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::shared_ptr<DSB_Deframer> def;

        int8_t *soft_buffer;

        int frame_count = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        NOAADSBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAADSBDecoderModule();
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
