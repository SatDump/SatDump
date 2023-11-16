#pragma once

#include "core/module.h"
#include "data/lrit_data.h"
#include "common/lrit/lrit_file.h"

namespace gk2a
{
    namespace lrit
    {
        class GK2ALRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            bool write_images;
            bool write_additional;
            bool write_unknown;

            std::string directory;

            enum CustomFileParams
            {
                JPG_COMPRESSED,
                J2K_COMPRESSED,
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

            std::map<std::string, SegmentedLRITImageDecoder> segmentedDecoders;
            std::map<std::string, std::unique_ptr<wip_images>> all_wip_images;

            std::map<int, uint64_t> decryption_keys;

            void processLRITFile(::lrit::LRITFile &file);

        public:
            GK2ALRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GK2ALRITDataDecoderModule();
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