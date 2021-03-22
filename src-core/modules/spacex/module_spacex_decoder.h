#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "modules/common/ccsds/ccsds_1_0_proba/deframer.h"
#include <dsp/random.h>

namespace spacex
{
    class SpaceXDecoderModule : public ProcessingModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        ccsds::ccsds_1_0_proba::CADUDeframer deframer;

        uint8_t rsWorkBuffer[255];
        int errors[5];

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // UI Stuff
        libdsp::Random rng;

    public:
        SpaceXDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~SpaceXDecoderModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace falcon