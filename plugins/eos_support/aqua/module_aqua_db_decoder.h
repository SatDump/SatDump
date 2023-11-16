#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>
#include "common/codings/deframing/bpsk_ccsds_deframer.h"

namespace aqua
{
    class AquaDBDecoderModule : public ProcessingModule
    {
    protected:
        uint8_t *buffer;

        int errors[4];
        deframing::BPSK_CCSDS_Deframer deframer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

    public:
        AquaDBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~AquaDBDecoderModule();
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
} // namespace aqua