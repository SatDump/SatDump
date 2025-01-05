#pragma once

#include "core/module.h"
#include "instruments/mwr/mwr_reader.h"
#include "instruments/navatt/navatt_reader.h"

namespace aws
{
    class AWSInstrumentsDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // Readers
        mwr::MWRReader mwr_reader;
        mwr::MWRReader mwr_dump_reader;
        navatt::NavAttReader navatt_reader;

        // Statuses
        instrument_status_t mwr_status = DECODING;
        instrument_status_t mwr_dump_status = DECODING;

    public:
        AWSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}