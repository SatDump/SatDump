#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/codings/viterbi/viterbi_1_2.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace fengyun3
{
    class FengyunMPTDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
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

        int v1, v2, vout;

        // Diff decoder input and output
        uint8_t *diff_out;

        viterbi::Viterbi1_2 viterbi1, viterbi2;
        deframing::BPSK_CCSDS_Deframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history1[200];
        float ber_history2[200];

    public:
        FengyunMPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~FengyunMPTDecoderModule();
        void process();
        void drawUI(bool window);

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace fengyun3