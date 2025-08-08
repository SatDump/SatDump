#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "xrit/processor/xrit_channel_processor.h"
#include "xrit/xrit_file.h"

namespace gk2a
{
    namespace lrit
    {
        class GK2ALRITDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            satdump::xrit::XRITChannelProcessor processor;

            bool write_images;
            bool write_additional;
            bool write_unknown;
            bool uhrit_mode;

            std::string directory;

            std::map<int, uint64_t> decryption_keys;

            void processLRITFile(satdump::xrit::XRITFile &file);

        public:
            GK2ALRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GK2ALRITDataDecoderModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace lrit
} // namespace gk2a