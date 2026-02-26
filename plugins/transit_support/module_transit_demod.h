#pragma once

#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "common/dsp/utils/real_to_complex.h"
#include "common/dsp/utils/freq_shift.h"
#include "common/dsp/utils/complex_to_mag.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/clock_recovery/clock_recovery_gardner.h"
#include "pipeline/modules/demod/module_demod_base.h"

// Handle the FM demodulation part of APT

namespace transit
{
    class TransitDemodModule : public satdump::pipeline::demod::BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::PLLCarrierTrackingBlock> carrier_pll;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
        std::shared_ptr<dsp::stream<float>> demod_stream;
        std::shared_ptr<dsp::RealToComplexBlock> rtc;

        std::shared_ptr<dsp::stream<complex_t>> fsk_stream;
        std::shared_ptr<dsp::FreqShiftBlock> fsk_fsb;
        std::shared_ptr<dsp::FIRBlock<complex_t>> fsk_lpf;
        std::shared_ptr<dsp::QuadratureDemodBlock> fsk_qua;
        std::shared_ptr<dsp::FIRBlock<float>> fsk_rrc;
        // std::shared_ptr<dsp::GardnerClockRecoveryBlock<float>> fsk_rec;
        std::shared_ptr<dsp::MMClockRecoveryBlock<float>> fsk_rec;

        std::shared_ptr<dsp::stream<complex_t>> usb_stream;
        std::shared_ptr<dsp::FreqShiftBlock> usb_fsb;
        std::shared_ptr<dsp::FIRBlock<complex_t>> usb_bpf;
        
        std::ofstream wav_out;
        
        bool play_audio;
        bool save_wav = false;
        bool save_soft = false;

        float display_freq = 0;

        // manually tuned, still not quite as good as Xerbos flowchart, but pretty close
        float d_clock_gain_omega = 0.0005;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = 0.8;
        float d_clock_omega_relative_limit = 0.005f;

        int8_t *sym_buffer;

    public:
        TransitDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~TransitDemodModule();
        void init();
        void stop();
        void process();
        void drawUI(bool window);

        bool enable_audio = false;

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; }
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);

        nlohmann::json getModuleStats();
    };
} // namespace transit