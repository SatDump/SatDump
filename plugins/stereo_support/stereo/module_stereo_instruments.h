#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

#include "image/image.h"
#include "instruments/secchi/secchi_demuxer.h"

#include "instruments/secchi/secchi_reader.h"

namespace stereo
{
    class StereoInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        secchi::PayloadAssembler secchi_assembler0, secchi_assembler1, secchi_assembler2, secchi_assembler3;

        image::Image decompress_icer_tool(uint8_t *data, int dsize, int size);

        secchi::SECCHIReader *secchi_reader;
        std::string icer_path;

    public:
        StereoInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace stereo