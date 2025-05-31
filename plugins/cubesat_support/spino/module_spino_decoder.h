#pragma once

#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace spino
{
    class SpinoDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        int8_t *input_buffer;

        int frm_cnt = 0;

    public:
        SpinoDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SpinoDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace spino
