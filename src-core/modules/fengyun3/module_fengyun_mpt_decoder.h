#pragma once

#include "module.h"
#include <complex>
#include <future>
#include "mpt_viterbi.h"
#include "common/ccsds/ccsds_1_0_1024/deframer.h"
#include <fstream>

namespace fengyun3
{
    class FengyunMPTDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;
        bool d_soft_symbols;

        uint8_t *viterbi_out;
        std::complex<float> *sym_buffer;
        int8_t *soft_buffer;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        // Viterbi output buffer
        uint8_t *viterbi1_out;
        uint8_t *viterbi2_out;

        // A few buffers for processing
        std::complex<float> *iSamples, *qSamples;
        int inI = 0, inQ = 0;

        bool d_invert_second_viterbi;

        std::future<void> v1_fut, v2_fut;

        int v1, v2;

        int diffin = 0;

        // Diff decoder input and output
        uint8_t *diff_in, *diff_out;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        FengyunMPTViterbi viterbi1, viterbi2;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history1[200];
        float ber_history2[200];

    public:
        FengyunMPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~FengyunMPTDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace fengyun