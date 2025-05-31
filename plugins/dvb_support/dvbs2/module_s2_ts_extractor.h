#pragma once

#include "pipeline/module.h"
#include <fstream>

namespace dvbs2
{
    class S2TStoTCPModule : public satdump::pipeline::ProcessingModule
    {
    protected:
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
        std::vector<satdump::pipeline::ModuleDataType> getInputTypes();
        std::vector<satdump::pipeline::ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace dvbs2
