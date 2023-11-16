#pragma once

#include "core/module.h"
#include "data/lrit_data.h"
#include "common/lrit/lrit_file.h"

extern "C"
{
#include <libs/aec/szlib.h>
}

namespace goes
{
    namespace hrit
    {
        class GOESLRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            bool write_images;
            bool write_emwin;
            bool write_messages;
            bool write_dcs;
            bool write_unknown;
            bool write_lrit;

            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_full_disk;
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_meso1;
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_meso2;
            std::map<int, SegmentedLRITImageDecoder> segmentedDecoders;

            std::string directory;

            enum CustomFileParams
            {
                RICE_COMPRESSED,
                FILE_APID,
            };

            std::map<std::string, SZ_com_t> rice_parameters_all;

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
            void saveLRITFile(::lrit::LRITFile &file, std::string path);

        public:
            GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GOESLRITDataDecoderModule();
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
    } // namespace hrit
} // namespace goes
