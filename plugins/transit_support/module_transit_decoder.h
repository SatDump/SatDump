#pragma once

#include "transit_deframer.h"

#include "common/repack_bits_byte.h"
#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace transit
{
    class TransitDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::shared_ptr<TransitDeframer> def;

        int8_t *soft_buffer;

        int frame_count = 0;
        uint32_t last_framecounter = 0;
        int coherent_frames = 0;
        bool framecounter_coherent = false;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        TransitDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~TransitDecoderModule();
        void process();
        void drawUI(bool window);

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; }
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace transit
