#pragma once

#include "dcs/dcs_decoder.h"
#include "goes/crc32.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "xrit/processor/xrit_channel_processor.h"
#include "xrit/xrit_file.h"

#include <deque>
#include <set>

extern "C"
{
#include <libs/aec/szlib.h>
}

namespace goes
{
    namespace hrit
    {
        class GOESLRITDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            bool write_images;
            bool write_emwin;
            bool parse_dcs;
            bool write_dcs;
            bool write_messages;
            bool write_unknown;
            bool write_lrit;

            // EMWIN-specific
            bool write_emwin_text;
            bool write_emwin_nws;

            bool fill_missing;
            int max_fill_lines;

            std::map<std::string, std::shared_ptr<satdump::xrit::XRITChannelProcessor>> all_processors;
            std::mutex all_processors_mtx;

            std::string directory;

            bool is_gui = false;
            CRC32 dcs_crc32;
            std::mutex ui_dcs_mtx;
            std::shared_ptr<DCSMessage> focused_dcs_message = nullptr;
            std::deque<std::shared_ptr<DCSMessage>> ui_dcs_messages;
            std::set<uint32_t> filtered_dcps;

            inline static std::map<std::string, std::string> shef_codes;
            inline static std::map<uint32_t, std::shared_ptr<DCP>> dcp_list;
            inline static std::mutex dcp_mtx;

            enum CustomFileParams
            {
                RICE_COMPRESSED,
                FILE_APID,
            };

            std::map<std::string, SZ_com_t> rice_parameters_all;

            void initDCS();
            void saveEMWINFile(std::string directory, std::string filename, char *buffer, size_t size);
            void processLRITFile(satdump::xrit::XRITFile &file);
            void saveLRITFile(satdump::xrit::XRITFile &file, std::string path);

            static void loadDCPs();

            bool processDCS(uint8_t *data, size_t size);
            void drawDCSUI();

        public:
            GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GOESLRITDataDecoderModule();
            void process();
            void drawUI(bool window);

        public:
            struct DCPUpdateEvent
            {
            };

            static void updateDCPs(DCPUpdateEvent);

            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace hrit
} // namespace goes
