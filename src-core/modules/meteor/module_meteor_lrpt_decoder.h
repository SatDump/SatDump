#pragma once

#include "module.h"
#include <complex>
#include "modules/common/viterbi/viterbi27.h"
#include <fstream>

namespace meteor
{
    class METEORLRPTDecoderModule : public ProcessingModule
    {
    protected:
        bool diff_decode;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        uint8_t *buffer;

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
        METEORLRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~METEORLRPTDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace meteor