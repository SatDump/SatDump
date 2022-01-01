#pragma once

#include "module.h"
#include "common/ccsds/ccsds_1_0_1024/deframer.h"
#include "common/codings/viterbi/viterbi_3_4.h"
#include <fstream>

namespace metop
{
    class MetOpAHRPTDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;

        uint8_t *viterbi_out;
        int8_t *soft_buffer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        viterbi::Viterbi3_4 viterbi;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history[200];

    public:
        MetOpAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~MetOpAHRPTDecoderModule();
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
} // namespace metop