#pragma once

#include "../../worker.h"
#include "image/image.h"
#include "simulator/instruments/imager_simulator.h"
#include "utils/time.h"

namespace satdump
{
    namespace ominous
    {
        class CSRSimulator : public InstrumentWorkerBase
        {
        public:
            CSRSimulator(WorkerCore core);
            void loop();

        private:
            ImagerSimulator simulator_csr;
            double start_time;

            int pkt_index_seq = 0;

            int test_channels_line = 0;
            image::Image test_channels[10];

            bool simulate_imagery = true;

        public:
            void setSimulatedImagery(bool v) { simulate_imagery = v; }
            bool getSimulatedImagery() { return simulate_imagery; }
        };
    } // namespace ominous
} // namespace satdump