#pragma once

#include "common/dsp/utils/random.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace fengyun_svissr
{
    class SVISSRDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        // Read buffer
        int8_t *buffer;

        // UI Stuff
        dsp::Random rng;

    public:
        SVISSRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SVISSRDecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace fengyun_svissr