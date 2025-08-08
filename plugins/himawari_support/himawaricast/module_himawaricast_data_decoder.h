#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "xrit/processor/xrit_channel_processor.h"

namespace himawari
{
    namespace himawaricast
    {
        class HimawariCastDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            std::string directory;

            satdump::xrit::XRITChannelProcessor processor;

        public:
            HimawariCastDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~HimawariCastDataDecoderModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace himawaricast
} // namespace himawari