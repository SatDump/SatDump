#pragma once

#include "common/lrit/lrit_file.h"
#include "common/lrit/lrit_productizer.h"
#include "data/lrit_data.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace gk2a
{
    namespace lrit
    {
        class GK2ALRITDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
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

            ::lrit::LRITProductizer productizer;
            void saveImageP(GK2AxRITProductMeta meta, image::Image img);

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