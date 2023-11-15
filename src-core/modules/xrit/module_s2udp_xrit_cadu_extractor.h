#pragma once

#include "core/module.h"
#include <fstream>

namespace xrit
{
    class S2UDPxRITCADUextractor : public ProcessingModule
    {
    protected:
        int bbframe_size;
        int pid_to_decode;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI Stuff
        int current_pid;

    public:
        S2UDPxRITCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~S2UDPxRITCADUextractor();
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
}
