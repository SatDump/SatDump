#pragma once

#include "module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "deframer.h"

namespace meteor
{
    class METEORHRPTDecoderModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<CADUDeframer> def;

        uint8_t *read_buffer;
        uint8_t *manchester_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

    public:
        METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~METEORHRPTDecoderModule();
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