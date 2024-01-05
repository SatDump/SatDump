#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/widgets/constellation.h"

namespace generic
{
    class Soft2HardModule : public ProcessingModule
    {
    protected:
        int8_t *input_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

    public:
        Soft2HardModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~Soft2HardModule();
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
} // namespace noaa
