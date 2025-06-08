#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/codings/viterbi/viterbi27.h"
#include "common/codings/viterbi/viterbi_1_2.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace meteor
{
    class METEORLRPTDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        bool diff_decode;

        int8_t *_buffer, *_buffer2;
        int8_t *buffer, *buffer2;

        bool m2x_mode;

        int viterbi_lock;
        float viterbi_ber;

        bool locked = false;
        int errors[4];
        int cor;

        std::shared_ptr<viterbi::Viterbi27> viterbi;

        std::shared_ptr<viterbi::Viterbi1_2> viterbin, viterbin2;
        std::shared_ptr<deframing::BPSK_CCSDS_Deframer> deframer;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

    public:
        METEORLRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~METEORLRPTDecoderModule();
        void process();
        void drawUI(bool window);

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace meteor