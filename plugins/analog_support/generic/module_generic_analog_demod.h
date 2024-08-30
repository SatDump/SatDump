#pragma once

#include "modules/demod/module_demod_base.h"
#include "common/dsp/demod/quadrature_demod.h"

namespace generic_analog
{
    class GenericAnalogDemodModule : public demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;


        bool nfm_demod = true;
        bool am_demod = false;

        
        bool settings_changed = false;
        int upcoming_symbolrate = 0;
        int e = 0;

        bool play_audio;
        uint64_t audio_samplerate = 48e3;

        std::mutex proc_mtx;

    public:
        GenericAnalogDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GenericAnalogDemodModule();
        void init();
        void stop();
        void process();
        void drawUI(bool window);

        bool enable_audio = false;

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}
