#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>

#include "instruments/ols/ols_rtd_reader.h"

namespace dmsp
{
    class DMSPInstrumentsModule : public ProcessingModule
    {
    protected:
        std::ifstream data_in;

        uint8_t rtd_frame[19];

        // Line CNT
        int ols_line_count = 0;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // Readers
        ols::OLSRTDReader ols_reader;

        // Statuses
        instrument_status_t ols_status = DECODING;

    public:
        DMSPInstrumentsModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~DMSPInstrumentsModule();
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
