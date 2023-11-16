#pragma once

#include "core/module.h"
#include "data/lrit_data.h"
#include "common/lrit/lrit_file.h"

namespace fy4
{
    namespace lrit
    {
        class FY4LRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

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

            void processLRITFile(::lrit::LRITFile &file);

        public:
            FY4LRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~FY4LRITDataDecoderModule();
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