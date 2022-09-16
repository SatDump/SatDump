#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "deframer.h"
#include "common/widgets/constellation.h"

namespace meteor
{
    class METEORHRPTDecoderModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<CADUDeframer> def;

        int8_t *soft_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~METEORHRPTDecoderModule();
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