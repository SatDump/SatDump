#pragma once

#include "core/module.h"
#include "common/codings/crc/crc_generic.h"

namespace tubin
{
    class TUBINDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

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
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}