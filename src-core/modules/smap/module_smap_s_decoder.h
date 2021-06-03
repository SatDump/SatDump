#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "common/ccsds/ccsds_1_0_proba/deframer.h"
#include "common/dsp/lib/random.h"

namespace smap
{
    class SMAPSDecoderModule : public ProcessingModule
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
        dsp::Random rng;

    public:
        SMAPSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~SMAPSDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace falcon