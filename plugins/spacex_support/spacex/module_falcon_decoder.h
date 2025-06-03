#pragma once

#include "common/dsp/utils/random.h"
#include "falcon_video_encoder.hpp"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace spacex
{
    class FalconDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    public:
        FalconDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~FalconDecoderModule();
        void process();
        void drawUI(bool window);

    protected:
        // std::ofstream data_out;

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);

    private:
#ifdef USE_VIDEO_ENCODER
        std::unique_ptr<FalconVideoEncoder> videoStreamEnc;
#endif
    };
} // namespace spacex
