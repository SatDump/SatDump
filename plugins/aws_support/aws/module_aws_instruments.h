#pragma once

#include "core/module.h"
#include "instruments/sterna/sterna_reader.h"
#include "instruments/navatt/navatt_reader.h"

namespace aws
{
    class AWSInstrumentsDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // Readers
        sterna::SternaReader sterna_reader;
        sterna::SternaReader sterna_dump_reader;
        navatt::NavAttReader navatt_reader;

        // Statuses
        instrument_status_t sterna_status = DECODING;
        instrument_status_t sterna_dump_status = DECODING;

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