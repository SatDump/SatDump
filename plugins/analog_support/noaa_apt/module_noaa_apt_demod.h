#pragma once

#include "modules/demod/module_demod_base.h"
#include "common/dsp/demod/quadrature_demod.h"

// Handle the FM demodulation part of APT

namespace noaa_apt
{
    class NOAAAPTDemodModule : public demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;

    public:
        NOAAAPTDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~NOAAAPTDemodModule();
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