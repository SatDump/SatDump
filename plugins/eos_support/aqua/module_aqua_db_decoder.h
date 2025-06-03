#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace aqua
{
    class AquaDBDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t *buffer;

        int errors[4];
        deframing::BPSK_CCSDS_Deframer deframer;

    public:
        AquaDBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~AquaDBDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace aqua