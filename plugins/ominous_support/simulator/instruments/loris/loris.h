#pragma once

#include "../../worker.h"
#include "image/image.h"
#include "simulator/instruments/imager_simulator.h"
#include "utils/time.h"

namespace satdump
{
    namespace ominous
    {
        class LORISSimulator : public InstrumentWorkerBase
        {
        public:
            LORISSimulator(WorkerCore core);
            void loop();

        private:
            ImagerSimulator simulator_csr;
            double start_time;

            int pkt_index_seq = 0;
        };
    } // namespace ominous
} // namespace satdump