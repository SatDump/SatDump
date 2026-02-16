#pragma once

#include "handlers/handler.h"

#include "dsp/agc/agc.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/displays/const_disp.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/filter/rrc.h"
#include "dsp/io/iq_source.h"
#include "dsp/path/splitter.h"
#include "dsp/pll/costas.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/notated_num.h"
#include "common/widgets/waterfall_plot.h"

#include <memory>
#include <thread>

#include "dsp/device/options_displayer_warper.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace handlers
    {
        class CycloHelperHandler : public Handler
        {
        public:
            std::shared_ptr<ndsp::IQSourceBlock> file_source;
            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter;
            std::shared_ptr<ndsp::FFTPanBlock> fft;
            std::shared_ptr<ndsp::AGCBlock<complex_t>> agc;
            std::shared_ptr<ndsp::FIRBlock<complex_t>> rrc;
            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter2;
            std::shared_ptr<ndsp::FFTPanBlock> fft2;
            std::shared_ptr<ndsp::CostasBlock> costas;
            std::shared_ptr<ndsp::MMClockRecoveryBlock<complex_t>> recovery;
            std::shared_ptr<ndsp::ConstellationDisplayBlock> constell;

        public:
            widgets::NotatedNum<uint64_t> samplerate = widgets::NotatedNum<uint64_t>("Samplerate", 6e6, "S/s");
            widgets::NotatedNum<uint64_t> symbolrate = widgets::NotatedNum<uint64_t>("Symbolrate", 2.00e6, "S/s");

            std::unique_ptr<ndsp::OptDisplayerWarper> agc_optdisp;
            std::unique_ptr<ndsp::OptDisplayerWarper> rrc_optdisp;

            enum mod_type_t
            {
                MOD_BPSK = 0,
                MOD_QPSK = 1,
            };

            mod_type_t mod_type = MOD_BPSK;

            void applyParams();

        public:
            std::shared_ptr<widgets::FFTPlot> fft_plot;
            std::shared_ptr<widgets::FFTPlot> fft_plot2;

            bool started = 0;

        public:
            CycloHelperHandler();
            ~CycloHelperHandler();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Cyclo Helper"; }

            std::string getID() { return "cyclo_helper_handler"; }
        };
    } // namespace handlers
} // namespace satdump
