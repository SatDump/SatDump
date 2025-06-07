#pragma once

#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "sd_deframer.h"
#include <fstream>

namespace goes
{
    namespace sd
    {
        class GOESNSDDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            std::shared_ptr<GOESN_SD_Deframer> def;

            int8_t *soft_buffer;
            uint8_t *soft_bits;
            uint8_t *output_frames;

            //  int frame_count = 0;

            // UI Stuff
            widgets::ConstellationViewer constellation;

        public:
            GOESNSDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GOESNSDDecoderModule();
            void process();
            void drawUI(bool window);
            nlohmann::json getModuleStats();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace sd
} // namespace goes