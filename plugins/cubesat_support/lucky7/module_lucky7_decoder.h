#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"

namespace lucky7
{
    class Lucky7DecoderModule : public ProcessingModule
    {
    protected:
        int8_t *soft_buffer;
        uint8_t *byte_buffers;

        std::ifstream data_in;
        std::ofstream data_out;

        int frm_cnt = 0;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        Lucky7DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~Lucky7DecoderModule();
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
