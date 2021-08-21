#pragma once

#include "module.h"
#include <complex>
#include "common/codings/viterbi/viterbi27.h"
#include <fstream>
#include "common/dsp/lib/random.h"

#include "stream_viterbi.h"
#include "common/ccsds/ccsds_1_0_1024/deframer.h"

namespace xrit
{
    class XRITDecoderModule : public ProcessingModule
    {
    protected:
        uint8_t *buffer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        bool is_bpsk;
        bool diff_decode;
        bool locked = false;
        int errors[4];
        int cor;

        viterbi::Viterbi27 viterbi;

        // QPSK Variant
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;
        xRITViterbi stream_viterbi;
        uint8_t *viterbi_out;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

        dsp::Random rng;

    public:
        XRITDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~XRITDecoderModule();
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
}