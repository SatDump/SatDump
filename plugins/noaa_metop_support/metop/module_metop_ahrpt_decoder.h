#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/codings/viterbi/viterbi_3_4.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace metop
{
    class MetOpAHRPTDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_thresold;

        uint8_t *viterbi_out;
        int8_t *soft_buffer;

        viterbi::Viterbi3_4 viterbi;
        deframing::BPSK_CCSDS_Deframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history[200];

    public:
        MetOpAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~MetOpAHRPTDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams()
        {
            nlohmann::json v; // TODOREWORk
            v["viterbi_outsync_after"] = 10;
            v["viterbi_ber_thresold"] = 0.28;
            return v;
        }
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace metop