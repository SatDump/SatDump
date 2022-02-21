#pragma once

#include "module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "noaa_deframer.h"
#include "common/widgets/constellation.h"

namespace noaa
{
    class NOAAHRPTDecoderModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<NOAADeframer> def;

        int8_t *soft_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        int frame_count = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        NOAAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAAHRPTDecoderModule();
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
