#pragma once

#include "core/module.h"

#include "instruments/seviri/seviri_reader.h"

namespace meteosat
{
    class MSGInstrumentsDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        std::shared_ptr<msg::SEVIRIReader> seviri_reader;

    public:
        MSGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}
