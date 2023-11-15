#pragma once

#include "core/module.h"

#include "instruments/secchi/secchi_demuxer.h"
#include "common/image/image.h"

#include "instruments/secchi/secchi_reader.h"

namespace stereo
{
    class StereoInstrumentsDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        secchi::PayloadAssembler secchi_assembler0, secchi_assembler1, secchi_assembler2, secchi_assembler3;

        image::Image<uint16_t> decompress_icer_tool(uint8_t *data, int dsize, int size);

        secchi::SECCHIReader *secchi_reader;

    public:
        StereoInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}