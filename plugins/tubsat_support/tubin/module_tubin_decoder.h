#pragma once

#include "common/codings/crc/crc_generic.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace tubin
{
    class TUBINDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        bool d_check_crc;

        codings::crc::GenericCRC crc_check = codings::crc::GenericCRC(16, 4129, 0xFFFF, 0x0000, false, false);

        bool crc_valid(uint8_t *cadu);

        std::map<uint64_t, std::vector<uint8_t>> all_files_vis;

    public:
        TUBINDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace tubin