#pragma once

#include "core/module.h"
#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include <fstream>
#include "common/codings/viterbi/viterbi_3_4.h"

namespace fengyun3
{
    class FengyunAHRPTDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;

        int8_t *soft_buffer;

        int8_t *q_soft_buffer;
        int8_t *i_soft_buffer;

        // Viterbi output buffer
        uint8_t *viterbi1_out;
        uint8_t *viterbi2_out;

        // A few buffers for processing
        bool d_invert_second_viterbi;

        int v1, v2, vout;

        // Diff decoder input and output
        uint8_t *diff_out;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        viterbi::Viterbi3_4 viterbi1, viterbi2;
        deframing::BPSK_CCSDS_Deframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history1[200];
        float ber_history2[200];

    public:
        FengyunAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~FengyunAHRPTDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace fengyun