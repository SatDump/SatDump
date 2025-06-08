#pragma once

#include "common/codings/crc/crc_generic.h"
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace geoscan
{
    class GEOSCANDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        int8_t *input_buffer;
        int d_frame_length;
        int d_thresold;

        std::unique_ptr<def::SimpleDeframer> deframer;

        int frm_cnt = 0;

        codings::crc::GenericCRC crc_check = codings::crc::GenericCRC(16, 0x8005, 0xFFFF, 0x0000, false, false);

    public:
        GEOSCANDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GEOSCANDecoderModule();
        void process();
        const uint8_t *PN9_MASK_Generator();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace geoscan
