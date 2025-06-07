#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/widgets/constellation.h"
#include "pipeline/module.h"
#include <fstream>

namespace noaa
{
    class NOAAGACDecoderModule : public satdump::pipeline::ProcessingModule
    {
    protected:
        const bool backward;

        std::shared_ptr<deframing::BPSK_CCSDS_Deframer> deframer;

        int8_t *soft_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        int frame_count = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        NOAAGACDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAAGACDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<satdump::pipeline::ModuleDataType> getInputTypes();
        std::vector<satdump::pipeline::ModuleDataType> getOutputTypes();

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace noaa
