#pragma once

#include "module.h"
#include <complex>
#include "viterbi.h"
#include "modules/common/ccsds/ccsds_1_0_1024/deframer.h"
#include <fstream>

namespace npp
{
    class NPPHRDDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;
        bool d_soft_symbols;

        int sw;

        uint8_t *viterbi_out;
        std::complex<float> *sym_buffer;
        int8_t *soft_buffer;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        HRDViterbi viterbi;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history[200];

    public:
        NPPHRDDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~NPPHRDDecoderModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace npp