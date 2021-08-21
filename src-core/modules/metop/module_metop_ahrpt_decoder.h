#pragma once

#include "module.h"
#include <complex>
#include "common/ccsds/ccsds_1_0_1024/deframer.h"
#include "viterbi.h"
#include <fstream>

namespace metop
{
    class MetOpAHRPTDecoderModule : public ProcessingModule
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

        MetopViterbi viterbi;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history[200];

    public:
        MetOpAHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~MetOpAHRPTDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace metop