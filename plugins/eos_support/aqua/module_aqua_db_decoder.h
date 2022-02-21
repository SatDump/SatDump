#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "common/codings/deframing/bpsk_ccsds_deframer.h"

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

        uint8_t *buffer;

        int errors[4];
        deframing::BPSK_CCSDS_Deframer deframer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

    public:
        AquaDBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~AquaDBDecoderModule();
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
} // namespace aqua