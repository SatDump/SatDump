#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "common/ccsds/ccsds_1_0_jason/deframer.h"

namespace cryosat
{
    class CRYOSATDumpDecoderModule : public ProcessingModule
    {
    protected:
        // Clamp symbols
        int8_t clamp(int8_t &x)
        {
            if (x >= 0)
            {
                return 1;
            }
            if (x <= -1)
            {
                return -1;
            }
            return x > 255.0 / 2.0;
        }

        uint8_t *buffer;

        int errors[5];
        ccsds::ccsds_1_0_jason::CADUDeframer deframer;

        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

    public:
        CRYOSATDumpDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~CRYOSATDumpDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace aqua