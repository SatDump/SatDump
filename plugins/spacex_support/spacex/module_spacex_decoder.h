#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>
#include "deframer.h"
#include "common/dsp/utils/random.h"

namespace spacex
{
    class SpaceXDecoderModule : public ProcessingModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        ccsds::ccsds_standard::CADUDeframer deframer;

        uint8_t rsWorkBuffer[255];
        int errors[5];

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        bool qpsk;

        // UI Stuff
        dsp::Random rng;

    public:
        SpaceXDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SpaceXDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace falcon