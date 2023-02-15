#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>
#include "common/dsp/utils/random.h"
#include "falcon_video_encoder.hpp"

namespace spacex
{
    class FalconDecoderModule : public ProcessingModule
    {
    public:
        FalconDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~FalconDecoderModule();
        void process();
        void drawUI(bool window);

    protected:
        std::ifstream data_in;
        std::ofstream data_out;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);

    private:
#ifdef USE_VIDEO_ENCODER
        std::unique_ptr<FalconVideoEncoder> videoStreamEnc;
#endif
    };
} // namespace falcon
