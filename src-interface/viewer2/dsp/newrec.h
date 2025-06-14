#pragma once

#include "dsp/displays/const_disp.h"
#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/io/iq_sink.h"
#include "dsp/path/splitter.h"
#include "handlers/handler.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"
#include <memory>

#include "task_queue.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace handlers
    {
        class NewRecHandler : public Handler
        {
        public:
            NewRecHandler();
            ~NewRecHandler();

            bool deviceRunning = false;
            ndsp::DeviceInfo curDeviceI;
            std::vector<ndsp::DeviceInfo> foundDevices;

            std::shared_ptr<ndsp::DeviceBlock> dev;
            ndsp::OptionsDisplayer options_displayer_test;

            std::shared_ptr<ndsp::SplitterBlock<complex_t>> splitter;

            std::shared_ptr<ndsp::FFTPanBlock> fftp;

            std::shared_ptr<widgets::FFTPlot> fft_plot;
            std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

            // TESTING
            bool const_present = false;
            std::shared_ptr<ndsp::ConstellationDisplayBlock> const_disp;

            bool recording = false;
            std::shared_ptr<ndsp::IQSinkBlock> iq_sink;

            TaskQueue taskq;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "NewRec TODOREWORK"; }

            std::string getID() { return "newrec_test_handler"; }
        };
    } // namespace handlers
} // namespace satdump
