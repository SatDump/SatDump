#include "hrpt_modulator.h"
#include "common/codings/ldpc/labrador/encoder.h"
#include "common/codings/randomization.h"
#include "simulator/ccsds/ccsds_muxer.h"
#include <cstdint>
#include <cstring>

namespace satdump
{
    namespace ominous
    {
        HRPTModulator::HRPTModulator(WorkerCore core) //
            : InstrumentWorkerBase(core),             //
              muxer_hrpt(200, 100),                   //
              qpsk(dsp::QPSK), bpsk(dsp::BPSK)
        {
            output_stream.type = ndsp::DSP_SAMPLE_TYPE_CF32;
            output_stream.fifo = std::make_shared<ndsp::DSPStream>(4);

            core.pkt_bus->register_handler<PushSpacePacketEvent>(
                [this](PushSpacePacketEvent e)
                {
                    while (muxer_hrpt.pushPacket(e.vcid, e.pkt))
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                });

            start_th();
        }

        HRPTModulator::~HRPTModulator() { output_stream.fifo->wait_enqueue(output_stream.fifo->newBufferTerminator()); }

        void HRPTModulator::loop()
        {
            int cnt = 1;

            uint8_t cadus[1024 * cnt];
            uint8_t cadus_bits[8192 * cnt];

            muxer_hrpt.work(cadus, cnt);

            for (int c = 0; c < cnt; c++)
            {
                rs.encode_interlaved(&cadus[c * 1024 + 4], true, 4);
                derand_ccsds(&cadus[c * 1024 + 4], 1020);

                uint8_t *cadu = &cadus[c * 1024];
                for (int bit = 0; bit < 8192; bit++)
                    cadus_bits[c * 8192 + bit] = (cadu[bit / 8] >> (7 - (bit % 8))) & 1;
            }

            if (qpsk_mode)
            {
                auto &fifo = output_stream.fifo;

                auto buf = fifo->newBufferSamples(8192 * cnt * 4, sizeof(complex_t));
                buf.size = cnt * (8192 / 2) * 4;

                for (int i = 0; i < cnt * (8192 / 2); i++)
                    for (int y = 0; y < 4; y++)
                        buf.getSamples<complex_t>()[i * 4 + y] = complex_t(cadus_bits[i * 2 + 1] ? 1 : -1, cadus_bits[i * 2 + 0] ? 1 : -1);
                // qpsk.mod(cadus_bits[i * 2 + 0] << 1 | cadus_bits[i * 2 + 1]);

                fifo->wait_enqueue(buf);
            }
            else
            {
                auto &fifo = output_stream.fifo;

                auto buf = fifo->newBufferSamples(8192 * cnt * 4, sizeof(complex_t));
                buf.size = cnt * 8192 * 4;

                for (int i = 0; i < cnt * 8192; i++)
                    for (int y = 0; y < 4; y++)
                        buf.getSamples<complex_t>()[i * 4 + y] = bpsk.mod(cadus_bits[i]);

                fifo->wait_enqueue(buf);
            }
        }
    } // namespace ominous
} // namespace satdump