#pragma once

#include "module.h"
#include <complex>
#include "common/codings/viterbi/viterbi27.h"
#include <fstream>
#include "common/dsp/lib/random.h"

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

        bool locked = false;
        int errors[5];
        int cor;

        viterbi::Viterbi27 viterbi;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

        dsp::Random rng;

    public:
        ProbaSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~ProbaSDecoderModule();
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
} // namespace proba