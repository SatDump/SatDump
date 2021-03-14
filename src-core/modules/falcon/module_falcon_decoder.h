#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include <dsp/random.h>

namespace falcon
{
    class FalconDecoderModule : public ProcessingModule
    {
    protected:
        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

    public:
        FalconDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~FalconDecoderModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace falcon