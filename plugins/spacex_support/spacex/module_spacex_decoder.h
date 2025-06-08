#pragma once

#include "common/dsp/utils/random.h"
#include "deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace spacex
{
    class SpaceXDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        ccsds::ccsds_tm::CADUDeframer deframer;

        uint8_t rsWorkBuffer[255];
        int errors[5];

        bool qpsk;

        // UI Stuff
        dsp::Random rng;

    public:
        SpaceXDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SpaceXDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace spacex