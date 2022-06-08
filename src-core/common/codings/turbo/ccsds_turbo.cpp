#include "ccsds_turbo.h"
#include <cstdlib>
#include <cmath>
#include <cstring>

namespace codings
{
    namespace turbo
    {
        CCSDSTurbo::CCSDSTurbo(turbo_base_t base, turbo_rate_t type)
            : d_base(base), d_code_type(type)
        {
            // d_pack = new blocks::kernel::pack_k_bits(8);
            d_info_length = base * 8;

            int p[8] = {31, 37, 43, 47, 53, 59, 61, 67};
            int k1 = 8;
            int k2 = base;

            d_pi = (int *)malloc(d_info_length * sizeof *d_pi);

            for (int s = 1; s <= d_info_length; ++s)
            {
                int m = (s - 1) % 2;
                int i = (int)floor((s - 1) / (2 * k2));
                int j = (int)floor((s - 1) / 2) - i * k2;
                int t = (19 * i + 1) % (k1 / 2);
                int q = t % 8 + 1;
                int c = (p[q - 1] * j + 21 * m) % k2;
                d_pi[s - 1] = 2 * (t + c * (k1 / 2) + 1) - m - 1;
            }

            int N_components_upper;
            int N_components_lower;

            d_backward = "0011";

            switch (d_code_type)
            {
            case RATE_1_2:
            {
                N_components_upper = 2;
                N_components_lower = 1;

                d_forward_upper[0] = "10011"; // systematic output
                d_forward_upper[1] = "11011";

                d_forward_lower[0] = "11011";
                // need to define puncturing pattern here maybe with a pointer to function
                // 110 101 110 101 110 101

                d_code1 = convcode_initialize((char **)d_forward_upper, (char *)d_backward, N_components_upper);
                d_code2 = convcode_initialize((char **)d_forward_lower, (char *)d_backward, N_components_lower);
                d_turbo = turbo_initialize(d_code1, d_code2, d_pi, d_info_length);
                d_rate = 1.0 / 2.0;
                d_encoded_length = d_turbo.encoded_length * 2 / 3;
                break;
            }
            case RATE_1_3:
            {
                N_components_upper = 2;
                N_components_lower = 1;

                d_forward_upper[0] = "10011"; // systematic output
                d_forward_upper[1] = "11011";

                d_forward_lower[0] = "11011"; // no need for puncturing

                d_code1 = convcode_initialize((char **)d_forward_upper, (char *)d_backward, N_components_upper);
                d_code2 = convcode_initialize((char **)d_forward_lower, (char *)d_backward, N_components_lower);
                d_turbo = turbo_initialize(d_code1, d_code2, d_pi, d_info_length);
                d_rate = 1.0 / 3.0;
                d_encoded_length = d_turbo.encoded_length;
                break;
            }
            case RATE_1_4:
            {
                N_components_upper = 3;
                N_components_lower = 1;

                d_forward_upper[0] = "10011"; // systematic output
                d_forward_upper[1] = "10101";
                d_forward_upper[2] = "11111";

                d_forward_lower[0] = "11011"; // no need for puncturing

                d_code1 = convcode_initialize((char **)d_forward_upper, (char *)d_backward, N_components_upper);
                d_code2 = convcode_initialize((char **)d_forward_lower, (char *)d_backward, N_components_lower);
                d_turbo = turbo_initialize(d_code1, d_code2, d_pi, d_info_length);
                d_rate = 1.0 / 4.0;
                d_encoded_length = d_turbo.encoded_length;
                break;
            }
            case RATE_1_6:
            {
                N_components_upper = 4;
                N_components_lower = 2;

                d_forward_upper[0] = "10011"; // systematic output
                d_forward_upper[1] = "11011";
                d_forward_upper[2] = "10101";
                d_forward_upper[3] = "11111";

                d_forward_lower[0] = "11011"; // no need for puncturing
                d_forward_lower[1] = "11111";

                d_code1 = convcode_initialize((char **)d_forward_upper, (char *)d_backward, N_components_upper);
                d_code2 = convcode_initialize((char **)d_forward_lower, (char *)d_backward, N_components_lower);
                d_turbo = turbo_initialize(d_code1, d_code2, d_pi, d_info_length);
                d_rate = 1.0 / 6.0;
                d_encoded_length = d_turbo.encoded_length;
                break;
            }
            }
        }

        CCSDSTurbo::~CCSDSTurbo()
        {
            delete[] d_pi;
        }

        void CCSDSTurbo::encode(uint8_t *frame, uint8_t *codeword)
        {
            int *bits_in = (int *)malloc(d_encoded_length * sizeof(int *));
            for (int i = 0; i < d_info_length / 8; i++)
                for (int j = 0; j < 8; j++)
                    bits_in[i * 8 + j] = (frame[i] & (0x80 >> j)) ? 1 : 0;

            int *encoded = turbo_encode(bits_in, d_turbo);

            uint8_t *encoded_u8 = (uint8_t *)malloc(d_encoded_length * sizeof(uint8_t *));
            if (d_code_type == RATE_1_2)
            {
                int j = 0;
                for (int i = 0; i < d_turbo.encoded_length; i++)
                {
                    if (puncturing(i))
                    {
                        encoded_u8[j] = encoded[i];
                        j++;
                    }
                }
            }
            else
            {
                for (int i = 0; i < d_encoded_length; i++)
                    encoded_u8[i] = encoded[i];
            }

            memset(codeword, 0, d_encoded_length / 8);

            for (int i = 0; i < d_encoded_length; i++)
                codeword[i / 8] = codeword[i / 8] << 1 | encoded_u8[i];
        }

        void CCSDSTurbo::decode(float *codeword, uint8_t *frame, int iterations)
        {
            d_turbo.interleaver = d_pi;

            const float *bits_in = codeword;

            double *bits_depunctured = (double *)malloc(sizeof(double) * d_turbo.encoded_length);
            if (d_code_type == RATE_1_2)
            {
                int j = 0;
                for (int i = 0; i < d_turbo.encoded_length; i++)
                {
                    if (puncturing(i))
                    {
                        bits_depunctured[i] = bits_in[j];
                        j++;
                    }
                    else
                    {
                        bits_depunctured[i] = 0.0;
                    }
                }
            }
            else
            {
                for (int i = 0; i < d_encoded_length; i++)
                    bits_depunctured[i] = bits_in[i];
            }

            int *decoded = turbo_decode(bits_depunctured, iterations, d_sigma * d_sigma, d_turbo);

            uint8_t *decoded_u8 = frame;
            for (int i = 0; i < d_info_length / 8; i++)
            {
                decoded_u8[i] = 0;
                for (int j = 0; j < 8; j++)
                    decoded_u8[i] |= decoded[i * 8 + j] ? (0x80 >> j) : 0;
            }

            free(bits_depunctured);
            free(decoded);
            // free(decoded_u8);
        }
    }
}