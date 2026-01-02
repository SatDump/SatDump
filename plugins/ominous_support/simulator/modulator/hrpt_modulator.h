#pragma once

#include "../worker.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/dsp/demod/constellation.h"
#include "dsp/block.h"
#include "simulator/ccsds/ccsds_muxer.h"

namespace satdump
{
    namespace ominous
    {
        class HRPTModulator : public InstrumentWorkerBase
        {
        private:
            CCSDSMuxer muxer_hrpt;

            dsp::constellation_t qpsk;
            dsp::constellation_t bpsk;

            reedsolomon::ReedSolomon rs = reedsolomon::ReedSolomon(reedsolomon::RS223);

            bool qpsk_mode = true;

        public:
            HRPTModulator(WorkerCore core);
            ~HRPTModulator();
            void loop();

            void set_qpsk_mode(bool v) { qpsk_mode = v; }

        public:
            ndsp::BlockIO output_stream;
        };
    } // namespace ominous
} // namespace satdump