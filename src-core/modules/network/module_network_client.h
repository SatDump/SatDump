#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>

namespace network
{
    class NetworkClientModule : public ProcessingModule
    {
    protected:
        uint8_t *buffer;

        std::string mode = "default";

        std::ofstream data_out;

        int pkt_size;
        std::string address;
        int port;

    public:
        NetworkClientModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NetworkClientModule();
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