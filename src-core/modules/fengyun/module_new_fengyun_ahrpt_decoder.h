#pragma once

#include "module.h"
#include <complex>
#include <future>
#include "ahrpt_viterbi_new.h"
#include "common/ccsds/ccsds_1_0_1024/deframer.h"
#include <fstream>

namespace fengyun
{
    class NewFengyunAHRPTDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;
        bool d_soft_symbols;
        bool d_invert_second_viterbi;

        uint8_t *viterbi_out;
        int8_t *soft_buffer;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        // Viterbi output buffer
        uint8_t *viterbi1_out;
        uint8_t *viterbi2_out;

        // A few buffers for processing
        int8_t *iSamples, *qSamples;

        int vout;

        int diffin = 0;

        // Diff decoder input and output
        uint8_t *diff_in, *diff_out;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        DualAHRPTViterbi2 viterbi;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history1[200];
        float ber_history2[200];

    public:
        NewFengyunAHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~NewFengyunAHRPTDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace fengyun