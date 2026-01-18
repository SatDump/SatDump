#pragma once

#include "../../worker.h"

namespace satdump
{
    namespace ominous
    {
        class NavAttSimulator : public InstrumentWorkerBase
        {
        public:
            NavAttSimulator(WorkerCore core) : InstrumentWorkerBase(core) { start_th(); }
            void loop();

        private:
            int pkt_index_seq = 0;
        };
    } // namespace ominous
} // namespace satdump