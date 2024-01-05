#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>

namespace oceansat
{
    class Oceansat2DBDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *buffer;

        int frame_count;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        uint8_t dqpsk_demod(int8_t *buffer)
        {
            bool a = buffer[0] > 0;
            bool b = buffer[1] > 0;

            if (a)
            {
                if (b)
                    return 0x0;
                else
                    return 0x3;
            }
            else
            {
                if (b)
                    return 0x1;
                else
                    return 0x2;
            }
        }

    public:
        Oceansat2DBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~Oceansat2DBDecoderModule();
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
} // namespace oceansat