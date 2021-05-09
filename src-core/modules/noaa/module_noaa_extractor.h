#pragma once

#include "module.h"
#include <thread>
#include <fstream>

namespace noaa
{
    class NOAAExtractorModule : public ProcessingModule
    {
    protected:

        uint16_t *buffer;
        uint8_t *frameBuffer;

        std::ifstream data_in;
        std::ofstream tip_out;
        std::ofstream aip_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

    public:
        NOAAExtractorModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~NOAAExtractorModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
}
