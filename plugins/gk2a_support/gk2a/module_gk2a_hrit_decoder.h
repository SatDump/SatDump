#pragma once

#include "core/module.h"
#include <complex>
#include "common/codings/viterbi/viterbi27.h"
#include <fstream>

namespace gk2a
{
    class GK2AHRITDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *buffer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        bool locked = false;
        int errors[4];
        int cor;

        viterbi::Viterbi27 viterbi;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

    public:
        GK2AHRITDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GK2AHRITDecoderModule();
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
} // namespace gk2a