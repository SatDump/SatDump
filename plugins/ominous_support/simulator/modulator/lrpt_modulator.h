#pragma once

#include "../worker.h"
#include "common/codings/ldpc/labrador/codes/codes.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/viterbi/cc_encoder.h"
#include "common/dsp/demod/constellation.h"
#include "common/dsp/utils/random.h"
#include "dsp/block.h"
#include "simulator/ccsds/ccsds_muxer.h"
#include "utils/binary.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace satdump
{
    namespace ominous
    {
        class Puncture34
        {
        private:
            std::vector<uint8_t> buf;

        public:
            int puncture(uint8_t *in, int inc, uint8_t *out)
            {
                buf.insert(buf.end(), in, in + inc);
                int ouc = 0, i = 0;
                for (i = 0; i < ((buf.size() / 24) * 24); i++)
                {
                    if (i % 6 == 0)
                        out[ouc++] = buf[i];
                    else if (i % 6 == 1)
                        out[ouc++] = buf[i];
                    else if (i % 6 == 3)
                        out[ouc++] = buf[i];
                    else if (i % 6 == 4)
                        out[ouc++] = buf[i];
                }
                buf.erase(buf.begin(), buf.begin() + i);
                return ouc;
            }
        };

        class LRPTModulator : public InstrumentWorkerBase
        {
        private:
            const int cadu_ldpc_size = 768;
            const int cadu_ldpc_size_dsb = 1024;
            const int cadu_payload_ldpc_size = 512;

            CCSDSMuxer muxer_lrpt;

            const int ldpc_codeword_size = 6144 + 64;
            const int ldpc_codeword_size_dsb = 8192 + 64;

            dsp::constellation_t qpsk;
            dsp::constellation_t bpsk;

            labrador::code_params_t ldpc_code;
            labrador::code_params_t ldpc_code_dsb;

            std::vector<uint8_t> ldpc_sync = unsigned_to_bitvec<uint64_t>(0x034776c7272895b0);

            bool lrpt_mode = true;

            reedsolomon::ReedSolomon rs = reedsolomon::ReedSolomon(reedsolomon::RS223);
            viterbi::CCEncoder cc_enc = viterbi::CCEncoder(8192, 7, 2, {79, 109});
            Puncture34 puncture;

        private:
            std::mutex allowed_vcaps_mtx;

            std::vector<std::pair<int, int>> allowed_vcaps = {
                {1, 1},                                                                                             //
                {5, 101}, {5, 102}, {5, 103}, {5, 104}, {5, 105}, {5, 106}, {5, 107}, {5, 108}, {5, 109}, {5, 110}, //
                //{6, 2},                                                                                             //
            };

            std::vector<std::pair<int, int>> allowed_vcaps_dsb = {
                //{1, 1}, //
                {6, 2}, //
            };

        public:
            void add_pkt_filter(int vcid, int apid)
            {
                del_pkt_filter(vcid, apid);

                allowed_vcaps_mtx.lock();

                if (lrpt_mode)
                    allowed_vcaps.push_back({vcid, apid});
                else
                    allowed_vcaps_dsb.push_back({vcid, apid});

                allowed_vcaps_mtx.unlock();
            }

            void del_pkt_filter(int vcid, int apid)
            {
                allowed_vcaps_mtx.lock();

                if (lrpt_mode)
                    allowed_vcaps.erase(std::remove_if(allowed_vcaps.begin(), allowed_vcaps.end(), [&](auto &v) { return (v.first == vcid) && (v.second == apid); }), allowed_vcaps.end());
                else
                    allowed_vcaps_dsb.erase(std::remove_if(allowed_vcaps_dsb.begin(), allowed_vcaps_dsb.end(), [&](auto &v) { return (v.first == vcid) && (v.second == apid); }),
                                            allowed_vcaps_dsb.end());

                allowed_vcaps_mtx.unlock();
            }

        public:
            LRPTModulator(WorkerCore core);
            ~LRPTModulator();
            void loop();

            void set_lrpt_mode(bool v) { lrpt_mode = v; }

        public:
            ndsp::BlockIO output_stream;
        };
    } // namespace ominous
} // namespace satdump