#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

#include "instruments/seviri/seviri_reader.h"

namespace meteosat
{
    class MSGInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::shared_ptr<msg::SEVIRIReader> seviri_reader;

    public:
        MSGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace meteosat
