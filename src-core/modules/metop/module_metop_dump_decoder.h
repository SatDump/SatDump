#pragma once

#include "module.h"
#include <complex>
#include <fstream>

namespace metop
{
    class MetOpDumpDecoderModule : public ProcessingModule
    {
    protected:
        uint8_t *buffer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        bool locked = false;
        int errors[4];
        int cor;

        // UI Stuff
        float cor_history[200];

    public:
        MetOpDumpDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~MetOpDumpDecoderModule();
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
} // namespace meteor