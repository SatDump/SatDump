#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"

namespace spino
{
    class SpinoDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *input_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        int frm_cnt = 0;

    public:
        SpinoDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SpinoDecoderModule();
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
