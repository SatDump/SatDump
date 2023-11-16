#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>

namespace ccsds
{
    class CCSDSTurboDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *buffer_soft;
        float *buffer_floats;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        std::string d_turbo_rate;
        int d_turbo_base;
        int d_turbo_iters;

        int d_codeword_size;
        int d_asm_size;
        int d_frame_size;

        float asm_softs[192];

        bool locked = false;
        bool crc_lock = false;
        float cor;

        // UI Stuff
        float cor_history[200];

        std::string window_name;

    public:
        CCSDSTurboDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~CCSDSTurboDecoderModule();
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