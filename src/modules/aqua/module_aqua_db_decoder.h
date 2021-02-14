#pragma once

#include "module.h"
#include <complex>

namespace aqua
{
    class AquaDBDecoderModule : public ProcessingModule
    {
    protected:
        // Work buffers
        uint8_t rsWorkBuffer[255];

        // Clamp symbols
        int8_t clamp(int8_t &x)
        {
            if (x >= 0)
            {
                return 1;
            }
            if (x <= -1)
            {
                return -1;
            }
            return x > 255.0 / 2.0;
        }

    public:
        AquaDBDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        void process();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace aqua