#pragma once

#include "core/module.h"
#include <fstream>

namespace dvbs2
{
    class S2TStoTCPModule : public ProcessingModule
    {
    protected:
        int port;
        int bbframe_size;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;


    public:
        S2TStoTCPModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~S2TStoTCPModule();
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
