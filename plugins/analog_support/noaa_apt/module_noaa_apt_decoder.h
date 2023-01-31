#pragma once

#include "core/module.h"

#include "common/dsp/utils/real_to_complex.h"
#include "common/dsp/utils/freq_shift.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/utils/complex_to_mag.h"

#include "common/image/image.h"

#define APT_IMG_WIDTH 2080
#define APT_MAX_LINES 2400
#define APT_IMG_OVERS 4

namespace noaa_apt
{
    class NOAAAPTDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        long d_audio_samplerate;

        std::shared_ptr<dsp::RealToComplexBlock> rtc;
        std::shared_ptr<dsp::FreqShiftBlock> frs;
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> rsp;
        std::shared_ptr<dsp::FIRBlock<complex_t>> lpf;
        std::shared_ptr<dsp::ComplexToMagBlock> ctm;

        image::Image<uint16_t> wip_apt_image;

        // UI Stuff
        instrument_status_t apt_status = DECODING;
        bool has_to_update = false;
        unsigned int textureID = 0;
        uint32_t *textureBuffer;

        // Functions
        image::Image<uint16_t> synchronize(int line_cnt);

    public:
        NOAAAPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAAAPTDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes() { return {DATA_FILE, DATA_STREAM}; }
        std::vector<ModuleDataType> getOutputTypes() { return {DATA_FILE}; }

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}