#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "deframer.h"
#include <dsp/random.h>

namespace falcon
{
    class FalconTLMDecoderModule : public ProcessingModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        CADUDeframer deframer;

        uint8_t rsWorkBuffer[255];
        int errors[5];

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // UI Stuff
        libdsp::Random rng;

    public:
        FalconTLMDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~FalconTLMDecoderModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace falcon