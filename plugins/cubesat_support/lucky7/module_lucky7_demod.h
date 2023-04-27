#pragma once

#include "modules/demod/module_demod_base.h"
#include "common/dsp/utils/random.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/codings/crc/crc_generic.h"

namespace lucky7
{
    class Lucky7DemodModule : public demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
        std::shared_ptr<dsp::CorrectIQBlock<float>> dcb2;
        std::shared_ptr<dsp::FIRBlock<float>> fil;

        float *shifting_buffer = nullptr;

        float corr_thresold;

        int oversampled_size = 0;
        std::vector<float> sync_buf;
        int to_skip_samples = 0;
        void process_sample(float &news);

        int frm_cnt = 0;

        codings::crc::GenericCRC crc_check = codings::crc::GenericCRC(16, 0x8005, 0xFFFF, 0x0000, false, false);

    public:
        Lucky7DemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~Lucky7DemodModule();
        virtual void drawUI(bool window);
        void init();
        void stop();
        void process();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}