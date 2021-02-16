#pragma once

#include "module.h"
#include <complex>

namespace meteor
{
    class METEORLRPTDecoderModule : public ProcessingModule
    {
    protected:
        bool diff_decode;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        void shiftWithConstantSize(uint8_t *arr, int pos, int length);

    public:
        METEORLRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        void process();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace metop