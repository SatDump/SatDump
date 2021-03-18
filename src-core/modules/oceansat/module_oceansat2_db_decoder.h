#pragma once

#include "module.h"
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
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

    public:
        Oceansat2DBDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~Oceansat2DBDecoderModule();
        void process();
        void drawUI();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace oceansat