#pragma once

#include "common/lrit/lrit_file.h"
#include "data/lrit_data.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "xrit/processor/xrit_channel_processor.h"

namespace elektro
{
    namespace lrit
    {
        class ELEKTROLRITDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            satdump::xrit::XRITChannelProcessor processor;

            std::string directory;

            struct wip_images
            {
                lrit_image_status imageStatus = RECEIVING;
                int img_width, img_height;

                // UI Stuff
                bool hasToUpdate = false;
                unsigned int textureID = 0;
                uint32_t *textureBuffer;
            };

            std::map<int, std::unique_ptr<wip_images>> all_wip_images;

            void processLRITFile(::lrit::LRITFile &file);

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