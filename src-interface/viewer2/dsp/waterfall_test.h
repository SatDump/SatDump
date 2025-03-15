#pragma once

#include "products2/handler/handler.h"

#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <thread>

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace viewer
    {
        class WaterfallTestHandler : public Handler
        {
        public:
            WaterfallTestHandler();
            ~WaterfallTestHandler();

            nng_socket socket;
            nng_dialer dialer;

            std::thread listenTh;

            float *fft_buffer;
            std::shared_ptr<widgets::FFTPlot> fft_plot;
            std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Waterfall"; }

            std::string getID() { return "waterfall_test_handler"; }
        };
    }
}