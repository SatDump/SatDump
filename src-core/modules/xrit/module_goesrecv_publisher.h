#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>

namespace xrit
{
    class GOESRecvPublisherModule : public ProcessingModule
    {
    protected:
        uint8_t *buffer;

        std::ifstream data_in;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        std::string address;
        int port;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

    public:
        GOESRecvPublisherModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GOESRecvPublisherModule();
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