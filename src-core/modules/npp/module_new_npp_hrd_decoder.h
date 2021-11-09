#pragma once

#include "module.h"
#include <complex>
#include "viterbi_new.h"
#include "common/ccsds/ccsds_1_0_1024/deframer.h"
#include <fstream>

namespace npp
{
    class NewNPPHRDDecoderModule : public ProcessingModule
    {
    protected:
        int d_viterbi_outsync_after;
        float d_viterbi_ber_threasold;

        uint8_t *viterbi_out;
        int8_t *soft_buffer;

        // Work buffers
        uint8_t rsWorkBuffer[255];

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        HRDViterbi2 viterbi;
        ccsds::ccsds_1_0_1024::CADUDeframer deframer;

        int errors[4];

        // UI Stuff
        float ber_history[200];

    public:
        NewNPPHRDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NewNPPHRDDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace npp