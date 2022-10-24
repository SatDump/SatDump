#include "viterbi_3_4.h"
#include <cstring>
#include "common/utils.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace viterbi
{
    Viterbi3_4::Viterbi3_4(float ber_threshold, int max_outsync, int buffer_size, bool fymode) : d_ber_thresold(ber_threshold),
                                                                                                 d_max_outsync(max_outsync),
                                                                                                 d_buffer_size(buffer_size),
                                                                                                 d_fymode(fymode),
                                                                                                 d_state(ST_IDLE),
                                                                                                 cc_decoder_ber(TEST_BITS_LENGTH * 1.5 / 2, 7, 2, {79, 109}),
                                                                                                 cc_encoder_ber(TEST_BITS_LENGTH * 1.5 / 2, 7, 2, {79, 109}),
                                                                                                 cc_decoder(buffer_size * 1.5 / 2, 7, 2, {79, 109})

    {
        soft_buffer = new uint8_t[d_buffer_size * 2];
        depunc_buffer = new uint8_t[d_buffer_size * 2];
        output_buffer = new uint8_t[d_buffer_size * 2];

        for (int p = 0; p < 2; p++)
            for (int o = 0; o < 2; o++)
                d_bers[p][o] = 10;
    }

    Viterbi3_4::~Viterbi3_4()
    {
        delete[] soft_buffer;
        delete[] depunc_buffer;
        delete[] output_buffer;
    }

    float Viterbi3_4::get_ber(uint8_t *raw, uint8_t *rencoded, int len)
    {
        float errors = 0, total = 0;
        for (int i = 0; i < len; i++)
        {
            if (raw[i] != 128)
            {
                errors += (raw[i] > 127) != rencoded[i];
                total++;
            }
        }

        return (errors / total) * 5;
    }

    int Viterbi3_4::depuncture(uint8_t *in, uint8_t *out, int size, bool shift)
    {
        int oo = 0;

        if (d_fymode)
        {
            for (int i = 0; i < size / 2; i++)
            {
                /*
                FY Puncturing is the usual 110 pattern
                */
#if 1
                if (shift ^ (i % 2 == 0))
                {
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = in[i * 2 + 1];
                }
                else
                {
                    out[oo++] = 128;
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = in[i * 2 + 1];
                    out[oo++] = 128;
                }
#else
                out[oo++] = in[i * 2 + 0];
                out[oo++] = in[i * 2 + 1];
                out[oo++] = 128;
#endif
            }
        }
        else
        {
            for (int i = 0; i < size / 2; i++)
            {
                /*
                ....while MetOp swaps symbols once in a while.

                Keeping them grouped in the below way ensure we always
                end up with an even symbol output count for the r=1/2 decoder.
                */
                if (shift ^ (i % 2 == 0))
                {
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = in[i * 2 + 1];
                }
                else
                {
                    out[oo++] = 128;
                    out[oo++] = in[i * 2 + 1];
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = 128;
                }
            }
        }

        return oo;
    }

    int Viterbi3_4::work(int8_t *input, int size, uint8_t *output)
    {
        if (d_state == ST_IDLE) // Search for a lock
        {
            d_ber = 10;
            for (int phase = 0; phase < (d_fymode ? 1 : 2); phase++) // For FengYun, where the second state is swapped, we do NOT need to handle phase shifts
            {
                memcpy(ber_test_buffer, input, TEST_BITS_LENGTH);                            // Copy over small buffer
                rotate_soft(ber_test_buffer, TEST_BITS_LENGTH, (phase_t)phase, false);       // Phase shift
                signed_soft_to_unsigned(ber_test_buffer, ber_soft_buffer, TEST_BITS_LENGTH); // Convert to softs for the viterbi

                for (int shift = 0; shift < 2; shift++) // Test 2 puncturing shifts
                {
                    depuncture(ber_soft_buffer, ber_depunc_buffer, TEST_BITS_LENGTH, shift); // Depuncture

                    cc_decoder_ber.work(ber_depunc_buffer, ber_decoded_buffer);  // Decode....
                    cc_encoder_ber.work(ber_decoded_buffer, ber_encoded_buffer); // ....then reencode for comparison

                    d_bers[phase][shift] = get_ber(ber_depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.5); // Compute BER between initial buffer and re-encoded

                    if ((d_ber == 10 && d_bers[phase][shift] < d_ber_thresold) || // Check for a lock initially
                        (d_ber < 10 && d_bers[phase][shift] < d_ber))             // And if we find a lower BER, prefer it
                    {
                        d_ber = d_bers[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;          // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;     // Set current phase
                        d_shift = shift;              // Set current puncturing shift
                        d_invalid = 0;                // Reset invalid BER count

                        memset(soft_buffer, 128, d_buffer_size * 2);
                        memset(depunc_buffer, 128, d_buffer_size * 2);
                    }
                }
            }
        }

        int out_n = 0; // Output bytes count

        if (d_state == ST_SYNCED) // Decode
        {
            rotate_soft((int8_t *)input, size, d_phase, false);          // Phase shift
            signed_soft_to_unsigned((int8_t *)input, soft_buffer, size); // Soft convertion
            depuncture(soft_buffer, depunc_buffer, size, d_shift);       // Depuncturing

            cc_decoder.work(depunc_buffer, output); // Decode entire buffer
            out_n = (size * 1.5) / 2;

            cc_encoder_ber.work(output, ber_encoded_buffer);                            // Re-encoded for a BER check
            d_ber = get_ber(depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.5); // Compute BER

            if (d_ber > d_ber_thresold) // Check current BER
            {
                d_invalid++;
                if (d_invalid > d_max_outsync) // If we get over our max unsynced thresold...
                    d_state = ST_IDLE;         // ...reset the decoder
            }
            else
            {
                d_invalid = 0; // Otherwise, reset current count
            }
        }

        return out_n;
    }

    float Viterbi3_4::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;
            for (int p = 0; p < 2; p++)
                for (int o = 0; o < 2; o++)
                    if (ber > d_bers[p][o])
                        ber = d_bers[p][o];

            return ber;
        }
    }

    int Viterbi3_4::getState()
    {
        return d_state;
    }
}
