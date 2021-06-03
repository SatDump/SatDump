#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/deframer.h"
#include "common/dsp/lib/random.h"

namespace elektro_arktika
{
    class RDASDecoderModule : public ProcessingModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // UI Stuff
        dsp::Random rng;

    public:
        RDASDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~RDASDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace elektro_arktika