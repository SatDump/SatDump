#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>

namespace ccsds
{
    class CCSDSTurboR6K8920DecoderModule : public ProcessingModule
    {
    protected:
        int8_t *buffer_soft;
        float *buffer_floats;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        int d_turbo_iters;

        float asm_softs[192];

        bool locked = false;
        bool crc_lock = false;
        float cor;

        // UI Stuff
        float cor_history[200];

    public:
        CCSDSTurboR6K8920DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~CCSDSTurboR6K8920DecoderModule();
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