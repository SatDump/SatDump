#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"

namespace geoscan
{
    class GEOSCANDataDecoderModule : public ProcessingModule
    {
    protected:
        uint8_t *frame_buffer;

        std::ifstream data_in;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

    public:
        GEOSCANDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GEOSCANDataDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace noaa
