#pragma once

#include "../../worker.h"
#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "simulator/instruments/imager_simulator.h"
#include "utils/time.h"

namespace satdump
{
    namespace ominous
    {
        class LORISProcessor : public InstrumentWorkerBase
        {
        public:
            LORISProcessor(WorkerCore core);
            void loop();

            void process(ccsds::CCSDSPacket &pkt);

        private:
            int pkt_index_seq = 0;

            std::mutex pkt_vec_mtx;
            std::vector<ccsds::CCSDSPacket> pkt_vec;
        };
    } // namespace ominous
} // namespace satdump