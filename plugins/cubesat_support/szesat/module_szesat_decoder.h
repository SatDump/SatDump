#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"
#include "common/codings/crc/crc_generic.h"
#include "common/codings/reedsolomon/reedsolomon.h"

namespace szesat
{
    class SZESATDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *input_buffer;
        int d_frame_length;
        int d_thresold;
        int d_check_crc;
        bool d_enable_rs;
        int rs_error = -1;
        int crc_ok;
        // uint8_t *framebuf_temporary;
        std::string lastReceivedPacketType;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        std::unique_ptr<def::SimpleDeframer> deframer;

        int frm_cnt = 0;
        int lofasz = 0;

        codings::crc::GenericCRC crc_check = codings::crc::GenericCRC(32, 0x04C11DB7, 0xFFFFFFFF, 0x00000000, false, false);

        enum RadioMessageTypes_t {
            RF_PING = 0x00,
            RF_TLM_req = 0x01,
            RF_TinyGS_req = 0x02,
            RF_RFLSH = 0x03,
            RF_TXLAS = 0x04,
            RF_IMGUP = 0x05,
            RF_IMGRQ = 0x06,
            RF_WFLSH = 0x07,
            RF_RWFLS = 0x08,
            RF_CRCPG = 0x09,
            RF_RESET = 0x0a,
            RF_EFLSH = 0x0d,
            RF_EPRRD = 0x0e,
            RF_EPRWD = 0x0f,



            RF_ACK_EFLSH = 0xf5,
            RF_EPRRD_DL = 0xf6,
            RF_CRCPG_DL = 0xf9,
            RF_IMGDL = 0xfa,
            RF_LASER_DL = 0xfb,
            RF_RFLSH_DL = 0xfc,
            RF_TinyGS_TLM_DL = 0xfd,
            RF_TLM_DL = 0xfe,
            RF_ACK = 0xff
        };
        std::string findMessageType(uint8_t number);

    public:
        SZESATDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SZESATDecoderModule();
        void process();
        const uint8_t* PN9_MASK_Generator();
        const uint8_t* PN9_MASK_Generator2();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace noaa
