#pragma once

#include "common/dsp/demod/quadrature_demod.h"
#include "noise_reduction.h"
#include "pipeline/modules/demod/module_demod_base.h"

// Handle the FM demodulation part of APT

namespace noaa_apt
{
    class NOAAAPTDemodModule : public satdump::pipeline::demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::AptNoiseReductionBlock> nr;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
        bool play_audio;
        bool save_wav = false;
        bool sdrpp_noise_reduction = true;

    public:
        NOAAAPTDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAAAPTDemodModule();
        void init();
        void stop();
        void process();
        void drawUI(bool window);

        bool enable_audio = false;

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace noaa_apt