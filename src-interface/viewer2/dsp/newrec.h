#pragma once

#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
#include "dsp/fft/fft_pan.h"
#include "handler/handler.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace viewer
    {
        class NewRecHandler : public Handler
        {
        public:
            NewRecHandler();
            ~NewRecHandler();

            std::shared_ptr<ndsp::DeviceBlock> dev;
            ndsp::OptionsDisplayer options_displayer_test;

            std::shared_ptr<ndsp::FFTPanBlock> fftp;

            std::shared_ptr<widgets::FFTPlot> fft_plot;
            std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "NewRec TODOREWORK"; }

            std::string getID() { return "newrec_test_handler"; }
        };
    } // namespace viewer
} // namespace satdump
