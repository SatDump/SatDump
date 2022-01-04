#pragma once

#include "module.h"
#include <fstream>
#include "common/dsp/random.h"
#include "common/codings/viterbi/viterbi_1_2.h"
#include "common/codings/deframing/bpsk_ccsds_deframer.h"

namespace proba
{
    class ProbaSDecoderModule : public ProcessingModule
    {
    protected:
        bool derandomize;

        void shiftWithConstantSize(uint8_t *arr, int pos, int length);

        uint8_t *buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        viterbi::Viterbi1_2 viterbi;
        deframing::BPSK_CCSDS_Deframer deframer;

        int errors[5];
        int cor;

        // UI Stuff
        float ber_history[200];

        dsp::Random rng;

    public:
        ProbaSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~ProbaSDecoderModule();
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
} // namespace proba