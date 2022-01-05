#pragma once

#include "module.h"
#include <complex>
#include "common/codings/viterbi/viterbi_1_2.h"
#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include <fstream>

namespace npp
{
    class NPPHRDDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;

        uint8_t *viterbi_out;
        int8_t *soft_buffer;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        viterbi::Viterbi1_2 viterbi;
        deframing::BPSK_CCSDS_Deframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history[200];

    public:
        NPPHRDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NPPHRDDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace npp