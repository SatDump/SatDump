#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "xrit/processor/xrit_channel_processor.h"
#include "xrit/xrit_file.h"

namespace elektro
{
    namespace lrit
    {
        class ELEKTROLRITDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            satdump::xrit::XRITChannelProcessor processor;

            std::string directory;

            void processLRITFile(satdump::xrit::XRITFile &file);

        public:
            ELEKTROLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~ELEKTROLRITDataDecoderModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace lrit
} // namespace elektro