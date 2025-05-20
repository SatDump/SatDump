#pragma once

#include "dsp/utils/cyclostationary_analysis.h"
#include "dsp/utils/delay_one_imag.h"
#include "dsp/utils/freq_shift.h"
#include "dsp/utils/real_to_complex.h"
#include "handlers/handler.h"

#include "dsp/agc/agc.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/const/const_disp.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/filter/rrc.h"
#include "dsp/io/file_source.h"
#include "dsp/path/splitter.h"
#include "dsp/pll/costas.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/notated_num.h"

#include <memory>

#include "dsp/device/options_displayer_warper.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace handlers
    {
        class JulsCycloHelperHandler : public Handler
        {
        public:
            std::shared_ptr<ndsp::FileSourceBlock> file_source;
            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter;
            std::shared_ptr<ndsp::FFTPanBlock> fft;
            std::shared_ptr<ndsp::AGCBlock<complex_t>> agc;
            std::shared_ptr<ndsp::FIRBlock<complex_t>> rrc;
            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter2;
            std::shared_ptr<ndsp::FFTPanBlock> fft2;
            std::shared_ptr<ndsp::CostasBlock> costas;
            std::shared_ptr<ndsp::MMClockRecoveryBlock<complex_t>> recovery;
            std::shared_ptr<ndsp::ConstellationDisplayBlock> constell;
            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter3;
            std::shared_ptr<ndsp::DelayOneImagBlock> delay_one_imag;
            std::shared_ptr<ndsp::ConstellationDisplayBlock> constell2;
            std::shared_ptr<ndsp::CyclostationaryAnalysis> cyclo;
            std::shared_ptr<ndsp::RealToComplexBlock> realcomp;
            std::shared_ptr<ndsp::FreqShiftBlock> freq_shift;

        public:
            widgets::NotatedNum<uint64_t> samplerate = widgets::NotatedNum<uint64_t>("Samplerate", 6e6, "S/s");
            widgets::NotatedNum<uint64_t> symbolrate = widgets::NotatedNum<uint64_t>("Symbolrate", 2.33e6, "S/s");

            std::unique_ptr<ndsp::OptDisplayerWarper> file_source_optdisp;
            std::unique_ptr<ndsp::OptDisplayerWarper> freq_shift_optdisp;
            std::unique_ptr<ndsp::OptDisplayerWarper> agc_optdisp;
            std::unique_ptr<ndsp::OptDisplayerWarper> rrc_optdisp;

            enum mod_type_t
            {
                MOD_BPSK = 0,
                MOD_QPSK = 1,
                MOD_8PSK = 2,
            };

            mod_type_t mod_type = MOD_BPSK;

            void applyParams();

        public:
            std::shared_ptr<widgets::FFTPlot> fft_plot;
            std::shared_ptr<widgets::FFTPlot> fft_plot2;

            bool started = 0;

        public:
            JulsCycloHelperHandler();
            ~JulsCycloHelperHandler();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Juls Cyclo Analysis"; }

            std::string getID() { return "juls_cyclo_analysis_handler"; }
        };
    } // namespace handlers
} // namespace satdump
