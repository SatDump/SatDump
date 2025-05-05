#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"
#include "common/codings/crc/crc_generic.h"

namespace geoscan
{
    class GEOSCANDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *input_buffer;
        int d_frame_length;
        int d_thresold;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        std::unique_ptr<def::SimpleDeframer> deframer;

        int frm_cnt = 0;

        codings::crc::GenericCRC crc_check = codings::crc::GenericCRC(16, 0x8005, 0xFFFF, 0x0000, false, false);

    public:
        GEOSCANDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GEOSCANDecoderModule();
        void process();
        const uint8_t* PN9_MASK_Generator();
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
