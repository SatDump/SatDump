#pragma once

#include "core/module.h"
#include "instruments/mws/mws_reader.h"
#include "instruments/navatt/navatt_reader.h"

namespace aws
{
    class AWSInstrumentsDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // Readers
        mws::MWSReader mws_reader;
        mws::MWSReader mws_dump_reader;
        navatt::NavAttReader navatt_reader;

        // Statuses
        instrument_status_t mws_status = DECODING;
        instrument_status_t mws_dump_status = DECODING;

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