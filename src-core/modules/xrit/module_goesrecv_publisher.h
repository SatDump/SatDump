#pragma once

#include "module.h"
#include <complex>
#include <fstream>

namespace xrit
{
    class GOESRecvPublisherModule : public ProcessingModule
    {
    protected:
        uint8_t *buffer;

        std::ifstream data_in;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        std::string address;
        int port;

        // UI Stuff
        float ber_history[200];
        float cor_history[200];

    public:
        GOESRecvPublisherModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~GOESRecvPublisherModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
}