#pragma once

#include "dsp/agc/agc.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"
#include "dsp/device/options_displayer.h"
#include "dsp/device/options_displayer_warper.h"
#include "dsp/resampling/rational_resampler.h"
#include "dsp/utils/blanker.h"
#include "dsp/utils/quadrature_demod.h"
#include "dsp/utils/vco.h"
#include "handlers/handler.h"
#include "utils/task_queue.h"
#include <complex.h>
#include <memory>

namespace satdump
{
    namespace handlers
    {
        class FMTestHandler : public Handler
        {
        private:
            TaskQueue taskq;

            bool deviceRunning = false;
            ndsp::DeviceInfo curDeviceI;
            std::vector<ndsp::DeviceInfo> foundDevices;

            std::shared_ptr<ndsp::DeviceBlock> dev;
            std::shared_ptr<ndsp::OptDisplayerWarper> options_displayer_dev;

        private:
            std::shared_ptr<ndsp::Block> tx_audio_source;
            std::shared_ptr<ndsp::RationalResamplerBlock<float>> tx_resamp;
            std::shared_ptr<ndsp::VCOBlock> tx_vco;
            std::shared_ptr<ndsp::BlankerBlock<complex_t>> tx_blanker;

            std::shared_ptr<ndsp::AGCBlock<complex_t>> rx_agc;
            std::shared_ptr<ndsp::RationalResamplerBlock<complex_t>> rx_resamp;
            std::shared_ptr<ndsp::QuadratureDemodBlock> rx_quad;
            std::shared_ptr<ndsp::Block> rx_audio_sink;

            bool is_txing = false;

        public:
            FMTestHandler();
            ~FMTestHandler();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "FM Test"; }

            std::string getID() { return "fm_test_handler"; }
        };
    } // namespace handlers
} // namespace satdump