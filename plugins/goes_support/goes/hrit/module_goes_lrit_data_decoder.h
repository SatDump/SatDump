#pragma once

#include "core/module.h"
#include "dcs/dcs_decoder.h"
#include "data/lrit_data.h"
#include "common/lrit/lrit_file.h"
#include "common/lrit/lrit_productizer.h"
#include "goes/crc32.h"

#include <set>
#include <deque>

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
            bool parse_dcs;
            bool write_dcs;
            bool write_messages;
            bool write_unknown;
            bool write_lrit;

            bool fill_missing;
            int max_fill_lines;

            std::map<int, SegmentedLRITImageDecoder> segmentedDecoders;

            std::string directory;

            bool is_gui = false;
            CRC32 dcs_crc32;
            std::mutex ui_dcs_mtx;
            std::shared_ptr<DCSMessage> focused_dcs_message = nullptr;
            std::deque<std::shared_ptr<DCSMessage>> ui_dcs_messages;
            std::map<std::string, std::string> shef_codes;
            std::map<uint32_t, std::shared_ptr<DCP>> dcp_list;
            std::set<uint32_t> filtered_dcps;

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

            void initDCS();
            bool processDCS(uint8_t *data, size_t size);
            void drawDCSUI();

            ::lrit::LRITProductizer productizer;

            void saveImageP(GOESxRITProductMeta meta, image::Image &img);

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
