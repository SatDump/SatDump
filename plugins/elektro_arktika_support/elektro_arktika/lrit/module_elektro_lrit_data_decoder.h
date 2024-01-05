#pragma once

#include "core/module.h"
#include "data/lrit_data.h"
#include "common/lrit/lrit_file.h"

namespace elektro
{
    namespace lrit
    {
        class ELEKTROLRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            std::shared_ptr<ELEKTRO221Composer> elektro_221_composer_full_disk;
            std::shared_ptr<ELEKTRO321Composer> elektro_321_composer_full_disk;
            std::map<int, SegmentedLRITImageDecoder> segmentedDecoders;

            std::string directory;

            enum CustomFileParams
            {
                JPEG_COMPRESSED,
                WT_COMPRESSED,
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

            std::map<int, std::unique_ptr<wip_images>> all_wip_images;

            void processLRITFile(::lrit::LRITFile &file);

        public:
            ELEKTROLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~ELEKTROLRITDataDecoderModule();
            void process();
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace avhrr
} // namespace metop