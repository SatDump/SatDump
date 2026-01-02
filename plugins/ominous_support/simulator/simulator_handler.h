#pragma once

#include "common/tracking/tracking.h"
#include "dsp/channel_model/channel_model_simple.h"
#include "dsp/io/nng_iq_sink.h"
#include "dsp/utils/throttle.h"
#include "handlers/handler.h"
#include "imgui/imgui.h"

#include "simulator/instruments/cheap/cheap.h"
#include "simulator/instruments/csr/csr.h"
#include "simulator/instruments/csr/csr_lrpt.h"
#include "simulator/instruments/eagle/eagle.h"
#include "simulator/instruments/loris/loris.h"
#include "simulator/instruments/loris/loris_proc.h"
#include "simulator/instruments/navatt/navatt.h"
#include "simulator/modulator/hrpt_modulator.h"
#include "simulator/modulator/lrpt_modulator.h"
#include "simulator/udp_test/lrpt_udp.h"
#include "simulator/worker.h"
#include <complex.h>
#include <memory>

#include "dsp/channel_model/model/new/channel_model_leo.h"

namespace satdump
{
    namespace ominous
    {
        class SimulatorHandler : public handlers::Handler
        {
        protected:
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

        public:
            SimulatorHandler();
            ~SimulatorHandler();

        private:
            WorkerCore core;

            int filter_apid, filter_vcid;

            double lrpt_signal_level = -60;
            double lrpt_noise_level = -60;
            double lrpt_freq_shift = 0;
            bool lrpt_use_model = false;

            std::shared_ptr<ChannelModelLEO> lrpt_model;

            double hrpt_signal_level = -60;
            double hrpt_noise_level = -60;
            double hrpt_freq_shift = 0;
            bool hrpt_use_model = false;

            std::shared_ptr<ChannelModelLEO> hrpt_model;

            double qth_lat = 0;
            double qth_lon = 0;
            double qth_alt = 0;

        private:
            // std::shared_ptr<NavAttSimulator> navatt_simulator;
            std::shared_ptr<CSRSimulator> csr_simulator;
            std::shared_ptr<LORISSimulator> loris_simulator;

            std::shared_ptr<CHEAPSimulator> cheap_simulator;
            std::shared_ptr<EAGLESimulator> eagle_simulator;

            std::shared_ptr<CSRLRPTSimulator> csr_lrpt_encoder;
            std::shared_ptr<LORISProcessor> loris_proc_encoder;

            std::shared_ptr<LRPTModulator> lrpt_modulator;
            std::shared_ptr<ndsp::ThrottleBlock<complex_t>> lrpt_throttle;
            std::shared_ptr<ndsp::ChannelModelSimpleBlock> lrpt_channel_model;
            std::shared_ptr<ndsp::NNGIQSinkBlock> lrpt_nngsink;

            std::shared_ptr<HRPTModulator> hrpt_modulator;
            std::shared_ptr<ndsp::ThrottleBlock<complex_t>> hrpt_throttle;
            std::shared_ptr<ndsp::ChannelModelSimpleBlock> hrpt_channel_model;
            std::shared_ptr<ndsp::NNGIQSinkBlock> hrpt_nngsink;

            std::shared_ptr<LRPTUdpSink> lrpt_udp_sink;

        public:
            std::string getID() { return "ominous_simulator_handler"; }
            std::string getName() { return "Ominous Simulator"; }
        };
    } // namespace ominous
}; // namespace satdump