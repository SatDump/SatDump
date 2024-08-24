#pragma once

#include "common/dsp/demod/pm_to_bpsk.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "common/dsp/utils/agc2.h"
#include "common/dsp/utils/complex_to_mag.h"
#include "common/dsp/utils/freq_shift.h"
#include "common/dsp/utils/real_to_complex.h"
#include "modules/demod/module_demod_base.h"
#include "common/dsp/demod/quadrature_demod.h"
#include <complex.h>
#include <memory>

namespace generic_analog
{
    class GenericAnalogDemodModule : public demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
	std::shared_ptr<dsp::PLLCarrierTrackingBlock> pll;
	std::shared_ptr<dsp::ComplexToMagBlock> ctm;
	std::shared_ptr<dsp::FreqShiftBlock> fsb;
	std::shared_ptr<dsp::FreqShiftBlock> fsb2;
	std::shared_ptr<dsp::FIRBlock<complex_t>> lpf;
	//std::shared_ptr<dsp::FIRBlock<float>> lpf;
	std::shared_ptr<dsp::FIRBlock<complex_t>> bpf;
	std::shared_ptr<dsp::AGCBlock<complex_t>> agc2;

	std::shared_ptr<dsp::RealToComplexBlock> rtc;
	std::shared_ptr<dsp::RealToComplexBlock> rtc2;
	std::shared_ptr<dsp::FIRBlock<float>> hpf;


	float d_pll_bw = 0.003;
	float d_pll_max_offset = 0.05;


	float display_freq = 0;

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
