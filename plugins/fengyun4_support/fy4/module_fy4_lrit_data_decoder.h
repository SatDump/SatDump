#pragma once

#include "data/lrit_data.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "xrit/xrit_file.h"

namespace fy4
{
    namespace lrit
    {
        class FY4LRITDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            std::string directory;

            enum CustomFileParams
            {
                IS_ENCRYPTED,
                KEY_INDEX,
            };

            struct wip_images
            {
                lrit_image_status imageStatus = RECEIVING;
                int img_width, img_height;

                // UI Stuff
                bool hasToUpdate = false;
                unsigned int textureID = 0;
                uint32_t *textureBuffer;
            };

            std::map<int, SegmentedLRITImageDecoder> segmentedDecoders;
            std::map<int, std::unique_ptr<wip_images>> all_wip_images;

#if 0
            std::map<int, uint64_t> decryption_keys;
#endif

            void processLRITFile(satdump::xrit::XRITFile &file);

        public:
            FY4LRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~FY4LRITDataDecoderModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace lrit
} // namespace fy4