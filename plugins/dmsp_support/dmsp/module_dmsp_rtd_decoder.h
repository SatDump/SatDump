#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "dmsp_deframer.h"
#include "common/widgets/constellation.h"

namespace dmsp
{
    class DMSPRTDDecoderModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<DMSP_Deframer> def;

        int8_t *soft_buffer;
        uint8_t *soft_bits;
        uint8_t *output_frames;

        std::ifstream data_in;
        std::ofstream data_out;

        //  int frame_count = 0;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        DMSPRTDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~DMSPRTDDecoderModule();
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
} // namespace noaa
