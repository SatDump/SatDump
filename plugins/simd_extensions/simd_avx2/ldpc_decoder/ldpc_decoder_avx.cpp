#include "ldpc_decoder_avx.h"
#include <cassert>

namespace codings
{
    namespace ldpc
    {
        LDPCDecoderAVX::LDPCDecoderAVX(Sparse_matrix pcm) : LDPCDecoder(pcm)
        {
            int max_deg = 0;
            for (size_t rows = 0; rows < pcm.get_n_rows(); rows++)
            {
                int deg = 0;
                std::vector<int> deg_pos;
                for (size_t cols = 0; cols < pcm.get_n_cols(); cols++)
                    if (pcm.at(rows, cols))
                        deg++;
                if (max_deg < deg)
                    max_deg = deg;
            }

            d_pcm_num_cn = pcm.get_n_rows();
            d_pcm_num_vn = pcm.get_n_cols();
            d_pcm_max_cn_degree = max_deg;
            d_pcm_num_edges = pcm.get_n_connections();

            d_vns = new __m256i[d_pcm_num_vn];
            d_vns_to_cn_msgs = new __m256i[d_pcm_max_cn_degree];
            d_cn_to_vn_msgs = new __m256i[d_pcm_num_cn * d_pcm_max_cn_degree];
            d_abs_msgs = new __m256i[d_pcm_max_cn_degree];

            d_vn_addr = new __m256i *[d_pcm_num_edges];
            d_row_pos_deg = new int[d_pcm_num_cn * 2];

            /* Precompute VN addresses in the VN buffer. Since each row has a different
             * degree the VN addresses are compacted. Keeping the offset and the degree
             * of each row is therefore required. */
            int row_base_idx = 0;
            auto mv = pcm;
            for (size_t row = 0; row < mv.get_n_rows(); row++)
            {
                int row_deg = 0;

                for (size_t col = 0; col < mv.get_n_cols(); col++)
                    if (mv.at(row, col))
                        row_deg++;

                assert(row_deg <= d_pcm_max_cn_degree);

                d_row_pos_deg[row * 2] = row_base_idx;
                d_row_pos_deg[row * 2 + 1] = row_deg;

                for (size_t col = 0; col < mv.get_n_cols(); col++)
                    if (mv.at(row, col))
                        d_vn_addr[row_base_idx++] = &(d_vns[col]);
            }
        }

        LDPCDecoderAVX::~LDPCDecoderAVX()
        {
            delete[] d_vns;
            delete[] d_vns_to_cn_msgs;
            delete[] d_abs_msgs;
            delete[] d_cn_to_vn_msgs;
            delete[] d_vn_addr;
            delete[] d_row_pos_deg;
        }

        int LDPCDecoderAVX::decode(uint8_t *out, const int8_t *in, int it)
        {
            int corrections = 0;

            /* The length of the input block should correspond to the length of a codeword. */
            // if (len != d_pcm->code.n)
            // {
            //     return -1;
            // }

            /* Copy the input codeword and give lowest LLR value to each punctured
             * bit. Also interleave */
            for (int i = 0; i < d_pcm_num_vn; i++)
            {
                // d_vns[i] = (int16_t)in[i];
                int16_t *ptrVar = (int16_t *)d_vns;
                for (int z = 0; z < 16; z++)
                    ptrVar[16 * i + z] = in[z * d_pcm_num_vn + i];
            }

            /* Init of CN to VN messages */
            for (int i = 0; i < d_pcm_num_cn * d_pcm_max_cn_degree; i++)
            {
                // d_cn_to_vn_msgs[i] = 0;
                d_cn_to_vn_msgs[i] = _mm256_set1_epi16(0);
            }

            /* Decode step */
            while (it--)
            {
                for (int cn_idx = 0; cn_idx < d_pcm_num_cn; cn_idx++)
                {
                    generic_cn_kernel(cn_idx);
                }
            }

            /* Hard decision & Deinterleave */
            for (int i = 0; i < d_pcm_num_vn; i++)
            {
                int16_t *ptrVar = (int16_t *)d_vns;
                for (int z = 0; z < 16; z++)
                {
                    out[z * d_pcm_num_vn + i] = (uint8_t)(ptrVar[16 * i + z] >= 0 ? 1 : 0);

                    if (i < d_pcm_num_vn - d_pcm_num_cn)
                        if ((out[z * d_pcm_num_vn + i] > 0) != (in[z * d_pcm_num_vn + i] > 0))
                            corrections++;
                }
            }

            return corrections;
        }

        void LDPCDecoderAVX::generic_cn_kernel(int cn_idx)
        {
            /* Given an indexed CN, gather the messages of all VNs connected
             * to that CN and determine a new estimation for each of the VNs. */

            cn_row_base = d_row_pos_deg[cn_idx * 2];
            cn_deg = d_row_pos_deg[cn_idx * 2 + 1];
            cn_offset = d_pcm_max_cn_degree * cn_idx;

            for (int vn_idx = 0; vn_idx < cn_deg; vn_idx++)
            {
                d_vns_to_cn_msgs[vn_idx] =
                    _mm256_sub_epi16(*d_vn_addr[cn_row_base + vn_idx], d_cn_to_vn_msgs[cn_offset + vn_idx]);
            }

            parity = _mm256_set1_epi16(0);
            min1 = _mm256_set1_epi16(UINT8_MAX);
            min2 = _mm256_set1_epi16(UINT8_MAX);

            if (cn_deg & 0x1)
                parity = _mm256_xor_si256(_mm256_set1_epi16(0xFFFF), parity);

            /* Compute the parity of all soft bits represented by the VNs to CN
             * messages, the absolute value of each soft bit, and the overall
             * first and second minimums. All these results are used later to
             * determine a new estimation sent back to each of the VNs connected
             * to the current CN. */
            for (int vn_idx = 0; vn_idx < cn_deg; vn_idx++)
            {
                msg = d_vns_to_cn_msgs[vn_idx];
                parity = _mm256_xor_si256(parity, msg);

                /* Bit-hack to compute the absolute value of the message */
                // abs_mask = msg >> (sizeof(msg) * 8 - 1);
                // abs_msg = (int16_t)((msg + abs_mask) ^ abs_mask);
                abs_msg = _mm256_abs_epi16(msg);

                /* Determine the first and second minimum */
                min2 = _mm256_min_epi16(_mm256_max_epi16(min1, abs_msg), min2);
                min1 = _mm256_min_epi16(abs_msg, min1);

                /* Keep the computed absolute value for later */
                d_abs_msgs[vn_idx] = abs_msg;
            }

            /* Compute a new soft bit estimation for each VN respectively and send it
             * in a new message.
             * Basically, we are updating the value of each adjacent VN of the CN given the values of
             * the other VNs. We are computing an error correction value that we add to the
             * current value of a VN. The sign of the error correction value has to follow a
             * critical property of LDPC codes, that is, the set of all soft bits of VNs
             * adjacent to a CN has even parity. Ex: a hard bit should have value '0' if
             * if the remaining has even parity, otherwise '1'.
             * Concretely, the min-sum variant for belief-propagation works as follows to compute
             * a new estimation :
             * 1) Take the minimum absolute value among all soft bits of all other VNs
             *    (the value of the VN to be updated is not taken into account) which
             *    will become the magnitude of the error correcting value.
             * 2) Compute the sign of the error correcting value (which will make a soft
             *    bit converge toward a negative or positive value and thus to a hard
             *    '0' or '1' bit)
             * 3) Add it to the current soft bit value of a VN */
            for (int vn_idx = 0; vn_idx < cn_deg; vn_idx++)
            {
                equ_wip = _mm256_sign_epi16(_mm256_set1_epi16(-1), _mm256_cmpeq_epi16(d_abs_msgs[vn_idx], min1));
                equ_min1 = _mm256_add_epi16(_mm256_xor_si256(_mm256_set1_epi16(0xFFFF), equ_wip), _mm256_set1_epi16(1));
                min = _mm256_or_si256(_mm256_and_si256(min1, _mm256_xor_si256(_mm256_set1_epi16(0xFFFF), equ_min1)), _mm256_and_si256(min2, equ_min1));
                sign = _mm256_xor_si256(parity, d_vns_to_cn_msgs[vn_idx]);

                /* Bit hack in order to multiply by the sign */
                new_msg = _mm256_sign_epi16(min, sign);

                /* Add error correction value */
                to_vn = _mm256_add_epi16(new_msg, d_vns_to_cn_msgs[vn_idx]);

                /* Save new soft bit value and CN to VN message */
                d_cn_to_vn_msgs[cn_offset + vn_idx] = new_msg;
                *d_vn_addr[cn_row_base + vn_idx] = to_vn;
            }
        }
    }
}
