#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include <dsp/fir_filter.h>
#include <dsp/carrier_pll_psk.h>
#include <dsp/clock_recovery_mm.h>
#include <dsp/moving_average.h>
#include <dsp/pipe.h>
#include <thread>
#include <fstream>
#include "deframer.h"

namespace meteor
{
    class METEORHRPTDecoderModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<CADUDeframer> def;

        uint8_t *read_buffer;
        uint8_t *manchester_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        size_t filesize;

    public:
        METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~METEORHRPTDecoderModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace meteor