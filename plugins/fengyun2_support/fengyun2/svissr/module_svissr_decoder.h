#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>
#include "common/dsp/utils/random.h"

namespace fengyun_svissr
{
    class SVISSRDecoderModule : public ProcessingModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // UI Stuff
        dsp::Random rng;

    public:
        SVISSRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SVISSRDecoderModule();
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
} // namespace elektro_arktika