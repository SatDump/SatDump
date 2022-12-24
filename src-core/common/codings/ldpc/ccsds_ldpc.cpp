#include "ccsds_ldpc.h"
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "common/dsp/buffer.h"
#include "make_ccsds.h"
#include "logger.h"

#define MAX_SIMD_SIZE 32

namespace codings
{
    namespace ldpc
    {
        CCSDSLDPC::CCSDSLDPC(ldpc_rate_t rate, int block_size)
            : d_rate(rate), d_block_size(block_size)
        {
            if (d_rate == RATE_7_8)
            {
                init_dec(ccsds_78::make_r78_code());

                d_frame_size = 8160;
                d_codeword_size = 8176;
                d_data_size = 7136;

                depunc_buffer_in = dsp::create_volk_buffer<int8_t>(d_codeword_size * MAX_SIMD_SIZE);
                depunc_buffer_ou = dsp::create_volk_buffer<uint8_t>(d_codeword_size * MAX_SIMD_SIZE);
            }
            else
            {
                ccsds_ar4ja::ar4ja_rate_t rate4 = ccsds_ar4ja::AR4JA_RATE_1_2;
                if (d_rate == RATE_1_2)
                    rate4 = ccsds_ar4ja::AR4JA_RATE_1_2;
                else if (d_rate == RATE_2_3)
                    rate4 = ccsds_ar4ja::AR4JA_RATE_2_3;
                else if (d_rate == RATE_4_5)
                    rate4 = ccsds_ar4ja::AR4JA_RATE_4_5;

                ccsds_ar4ja::ar4ja_blocksize_t block4 = ccsds_ar4ja::AR4JA_BLOCK_1024;
                if (d_block_size == 1024)
                    block4 = ccsds_ar4ja::AR4JA_BLOCK_1024;
                else if (d_block_size == 4096)
                    block4 = ccsds_ar4ja::AR4JA_BLOCK_4096;
                else if (d_block_size == 16384)
                    block4 = ccsds_ar4ja::AR4JA_BLOCK_16384;
                else
                    throw std::runtime_error("This blocksize is not supported!");

                auto pcm = ccsds_ar4ja::make_ar4ja_code(rate4, block4, &d_M);
                init_dec(pcm);

                d_frame_size = pcm.get_n_cols() - d_M; // Punctured codeword size
                d_codeword_size = pcm.get_n_cols();    // Full codeword size
                d_data_size = pcm.get_n_rows() - d_M;  // Data words size

                depunc_buffer_in = dsp::create_volk_buffer<int8_t>(d_codeword_size * MAX_SIMD_SIZE);
                depunc_buffer_ou = dsp::create_volk_buffer<uint8_t>(d_codeword_size * MAX_SIMD_SIZE);
            }
        }

        void CCSDSLDPC::init_dec(Sparse_matrix pcm)
        {
            ldpc_decoder = get_best_ldpc_decoder(pcm);

            d_simd = ldpc_decoder->simd();
            d_is_geneneric = d_simd == 1;
            if (d_is_geneneric)
                d_simd = 1;
        }

        CCSDSLDPC::~CCSDSLDPC()
        {
            delete ldpc_decoder;
            volk_free(depunc_buffer_in);
            volk_free(depunc_buffer_ou);
        }

        int CCSDSLDPC::decode(int8_t *codeword, uint8_t *frame, int iterations)
        {
            if (d_rate == RATE_7_8)
            {
                for (int i = 0; i < d_simd; i++)
                {
                    memcpy(&depunc_buffer_in[i * d_codeword_size + 18], &codeword[i * d_frame_size], 8158);
                    for (int i = 0; i < d_simd; i++) // First 18s are 0s
                        memset(&depunc_buffer_in[i * d_codeword_size], 0, 18);
                }
            }
            else
            {
                for (int i = 0; i < d_simd; i++)
                {
                    memcpy(&depunc_buffer_in[i * d_codeword_size], &codeword[i * d_frame_size], d_frame_size);
                    for (int i = 0; i < d_simd; i++) // Last d_M bits are 0s
                        memset(&depunc_buffer_in[i * d_codeword_size + d_frame_size], 0, d_M);
                }
            }

            if (d_is_geneneric)
                for (int i = 0; i < d_simd; i++)
                    d_corr_errors += ldpc_decoder->decode(&depunc_buffer_ou[i * d_codeword_size], &depunc_buffer_in[i * d_codeword_size], iterations);
            else
                d_corr_errors = ldpc_decoder->decode(depunc_buffer_ou, depunc_buffer_in, iterations);

            d_corr_errors = d_corr_errors / d_simd;

            if (d_rate == RATE_7_8)
            {
                for (int i = 0; i < d_simd; i++)
                {
                    memcpy(&frame[i * d_frame_size], &depunc_buffer_ou[i * d_codeword_size + 18], 8158);
                }
            }
            else
            {
                for (int i = 0; i < d_simd; i++)
                {
                    memcpy(&frame[i * d_frame_size], &depunc_buffer_ou[i * d_codeword_size], d_frame_size);
                }
            }

            return d_corr_errors;
        }

        ldpc_rate_t ldpc_rate_from_string(std::string str)
        {
            if (str == "1/2")
                return RATE_1_2;
            else if (str == "2/3")
                return RATE_2_3;
            else if (str == "4/5")
                return RATE_4_5;
            else if (str == "7/8")
                return RATE_7_8;
            else
                throw std::runtime_error("Invalid LDPC code rate " + str);
        }
    }
}