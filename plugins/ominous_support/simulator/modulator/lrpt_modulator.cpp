#include "lrpt_modulator.h"
#include "common/codings/ldpc/labrador/encoder.h"
#include "common/codings/randomization.h"
#include "logger.h"
#include "simulator/ccsds/ccsds_muxer.h"
#include <chrono>
#include <complex.h>
#include <cstdint>
#include <cstring>
#include <thread>

namespace satdump
{
    namespace ominous
    {
        LRPTModulator::LRPTModulator(WorkerCore core) //
            : InstrumentWorkerBase(core),             //
              muxer_lrpt(20, 10),                     //, cadu_ldpc_size + 4, cadu_payload_ldpc_size - 12 + 4), //
              qpsk(dsp::QPSK), bpsk(dsp::BPSK)
        {
            output_stream.type = ndsp::DSP_SAMPLE_TYPE_CF32;
            output_stream.fifo = std::make_shared<ndsp::DSPStream>(2);

            ldpc_code = satdump::labrador::get_code_params(satdump::labrador::TM6144);
            ldpc_code_dsb = satdump::labrador::get_code_params(satdump::labrador::TM8192);

            core.pkt_bus->register_handler<PushSpacePacketEvent>(
                [this](PushSpacePacketEvent e)
                {
                    bool valid = false;

                    allowed_vcaps_mtx.lock();
                    if (lrpt_mode)
                    {
                        for (auto &v : allowed_vcaps)
                            if (v.first == e.vcid && v.second == e.pkt.header.apid)
                                valid = true;
                    }
                    else
                    {
                        for (auto &v : allowed_vcaps_dsb)
                            if (v.first == e.vcid && v.second == e.pkt.header.apid)
                                valid = true;
                    }
                    allowed_vcaps_mtx.unlock();

                    if (valid)
                        while (muxer_lrpt.pushPacket(e.vcid, e.pkt))
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                });

            start_th();
        }

        LRPTModulator::~LRPTModulator() { output_stream.fifo->wait_enqueue(output_stream.fifo->newBufferTerminator()); }

        void LRPTModulator::loop()
        {
            if (lrpt_mode) // LRPT Mode
            {
                if (false)
                { // LDPC 2/3
                    uint8_t cadus[cadu_ldpc_size + 4];
                    uint8_t cadus_bits[ldpc_codeword_size];

                    int cnt = 1;
                    muxer_lrpt.work(cadus, cnt);

                    for (int c = 0; c < cnt; c++)
                    {
                        uint8_t *cadu = &cadus[c * (cadu_ldpc_size + 4) + 4];

                        labrador::encode_parity_u32(ldpc_code, cadu);

                        derand_ccsds(cadu, 6144 / 8);

                        for (int bit = 0; bit < cadu_ldpc_size * 8; bit++)
                            cadus_bits[64 + bit] = (cadu[bit / 8] >> (7 - (bit % 8))) & 1;

                        memcpy(&cadus_bits[0], ldpc_sync.data(), 64);
                    }

                    auto &fifo = output_stream.fifo;

                    auto buf = fifo->newBufferSamples(ldpc_codeword_size * cnt * 4, sizeof(complex_t));
                    buf.size = cnt * (ldpc_codeword_size / 2) * 4;

                    for (int i = 0; i < cnt * (ldpc_codeword_size / 2); i++)
                        for (int y = 0; y < 4; y++)
                            buf.getSamples<complex_t>()[i * 4 + y] = qpsk.mod(cadus_bits[i * 2 + 1] << 1 | cadus_bits[i * 2 + 0]);

                    fifo->wait_enqueue(buf);
                }
                else
                { // RS + Vit 3/4
                    uint8_t cadus[1024];
                    uint8_t cadus_bits_previt[1024 * 8];
                    uint8_t cadus_bits[1024 * 8 * 2];
                    uint8_t cadus_bits2[1024 * 8 * 2];

                    int cnt = 1;
                    muxer_lrpt.work(cadus, cnt);

                    rs.encode_interlaved(cadus + 4, true, 4);
                    derand_ccsds(cadus + 4, 1020);

                    for (int bit = 0; bit < 1024 * 8; bit++)
                        cadus_bits_previt[bit] = (cadus[bit / 8] >> (7 - (bit % 8))) & 1;

                    cc_enc.work(cadus_bits_previt, cadus_bits, cnt * 8192);

                    auto &fifo = output_stream.fifo;

                    auto buf = fifo->newBufferSamples(1024 * 8 * 2 * cnt * 4, sizeof(complex_t));

                    int nbits = 0;

#if 0
                    for (int i = 0; i < 8192 * 2; i++)
                    {
                        if (i % 4 == 0)
                            cadus_bits2[nbits++] = cadus_bits[i];
                        else if (i % 4 == 1)
                            cadus_bits2[nbits++] = cadus_bits[i];
                        if (i % 4 == 3)
                            cadus_bits2[nbits++] = cadus_bits[i];
                    }
#else
                    nbits = puncture.puncture(cadus_bits, 8192 * 2, cadus_bits2);
#endif

                    for (int i = 0; i < cnt * (nbits / 2); i++)
                        for (int y = 0; y < 4; y++)
                            buf.getSamples<complex_t>()[i * 4 + y] = complex_t(cadus_bits2[i * 2 + 1] ? 1 : -1, cadus_bits2[i * 2 + 0] ? 1 : -1);
                    // qpsk.mod(cadus_bits2[i * 2 + 1] << 1 | cadus_bits2[i * 2 + 0]) * (1. - noise_ratio) + complex_t(rng.gasdev(), rng.gasdev()) * noise_ratio;

                    buf.size = (nbits / 2) * 4;

                    fifo->wait_enqueue(buf);
                }
            }
            else
            {
                if (false)
                { // LDPC 1/2
                    uint8_t cadus[cadu_ldpc_size_dsb + 4];
                    uint8_t cadus_bits[ldpc_codeword_size_dsb];

                    int cnt = 1;
                    muxer_lrpt.work(cadus, cnt);

                    for (int c = 0; c < cnt; c++)
                    {
                        uint8_t *cadu = &cadus[c * (cadu_ldpc_size_dsb + 4) + 4];

                        labrador::encode_parity_u32(ldpc_code_dsb, cadu);

                        derand_ccsds(cadu, 8192 / 8);

                        for (int bit = 0; bit < cadu_ldpc_size_dsb * 8; bit++)
                            cadus_bits[64 + bit] = (cadu[bit / 8] >> (7 - (bit % 8))) & 1;

                        memcpy(&cadus_bits[0], ldpc_sync.data(), 64);
                    }

                    auto &fifo = output_stream.fifo;

                    auto buf = fifo->newBufferSamples(ldpc_codeword_size_dsb * cnt * 40, sizeof(complex_t));
                    buf.size = cnt * ldpc_codeword_size_dsb * 40;

                    for (int i = 0; i < cnt * ldpc_codeword_size_dsb; i++)
                        for (int y = 0; y < 40; y++)
                            buf.getSamples<complex_t>()[i * 40 + y] = bpsk.mod(cadus_bits[i]);

                    fifo->wait_enqueue(buf);
                }
                else
                {
                    uint8_t cadus[1024];
                    uint8_t cadus_bits_previt[1024 * 8];
                    uint8_t cadus_bits[1024 * 8 * 2];
                    uint8_t cadus_manch[1024 * 8 * 2 * 2];

                    int cnt = 1;
                    muxer_lrpt.work(cadus, cnt);

                    rs.encode_interlaved(cadus + 4, true, 4);
                    derand_ccsds(cadus + 4, 1020);

                    for (int bit = 0; bit < 1024 * 8; bit++)
                        cadus_bits_previt[bit] = (cadus[bit / 8] >> (7 - (bit % 8))) & 1;

                    cc_enc.work(cadus_bits_previt, cadus_bits, cnt * 8192);

                    int nbits = 8192 * 4;
                    complex_t *output_symbols = new complex_t[8192 * 4 * 20];

                    for (int i = 0; i < 8192 * 2; i++)
                    {
                        cadus_manch[i * 2 + 0] = cadus_bits[i] ^ 0;
                        cadus_manch[i * 2 + 1] = cadus_bits[i] ^ 1;
                    }

                    for (int i = 0; i < cnt * nbits; i++)
                        for (int y = 0; y < 20; y++)
                            output_symbols[i * 20 + y] = (bpsk.mod(cadus_manch[i]) + complex_t(0, 1));

                    for (int i = 0; i < nbits * 20; i += 8192)
                    {
                        auto &fifo = output_stream.fifo;
                        auto buf = fifo->newBufferSamples(8192 * 2, sizeof(complex_t));
                        buf.size = 8192;

                        for (int y = 0; y < 8192; y++)
                            buf.getSamples<complex_t>()[y] = output_symbols[i + y];

                        fifo->wait_enqueue(buf);
                    }

                    delete[] output_symbols;
                }
            }
        }
    } // namespace ominous
} // namespace satdump