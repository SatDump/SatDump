#include "viterbi_punc.h"
#include <cstring>
#include "common/utils.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace viterbi
{
    Viterbi_Depunc::Viterbi_Depunc(std::shared_ptr<puncturing::GenericDepunc> depunc,
                                   float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases, bool check_iq_swap)
        : depunc(depunc),

          d_ber_thresold(ber_threshold),
          d_max_outsync(max_outsync),
          d_check_iq_swap(check_iq_swap),
          d_buffer_size(buffer_size),
          d_phases_to_check(phases),
          d_state(ST_IDLE),

          cc_decoder_ber(TEST_BITS_LENGTH, 7, 2, {79, 109}),
          cc_encoder_ber(TEST_BITS_LENGTH, 7, 2, {79, 109}),

          cc_decoder(buffer_size, 7, 2, {79, 109})
    {
        soft_buffer = new uint8_t[d_buffer_size * 8];
        depunc_buffer = new uint8_t[d_buffer_size * 8];
        output_buffer = new uint8_t[d_buffer_size * 8];

        for (int s = 0; s < 2; s++)
            for (int i = 0; i < 12; i++)
                for (int y = 0; y < 2; y++)
                    d_bers[s][i][y] = 10;
    }

    Viterbi_Depunc::~Viterbi_Depunc()
    {
        delete[] soft_buffer;
        delete[] depunc_buffer;
        delete[] output_buffer;
    }

    float Viterbi_Depunc::get_ber(uint8_t *raw, uint8_t *rencoded, int len, float ratio)
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

        return (errors / total) * ratio;
    }

    int Viterbi_Depunc::work(int8_t *input, int size, uint8_t *output)
    {
        if (d_state == ST_IDLE) // Search for a lock
        {
            d_ber = 10;
            for (int s = 0; s < (d_check_iq_swap ? 2 : 1); s++)
            {
                for (phase_t phase : d_phases_to_check)
                {
                    memcpy(ber_test_buffer, input, TEST_BITS_LENGTH);                            // Copy over small buffer
                    rotate_soft(ber_test_buffer, TEST_BITS_LENGTH, PHASE_0, s);                  // IQ Swap
                    rotate_soft(ber_test_buffer, TEST_BITS_LENGTH, phase, false);                // Phase shift
                    signed_soft_to_unsigned(ber_test_buffer, ber_soft_buffer, TEST_BITS_LENGTH); // Convert to softs for the viterbi

                    // Puncturing stuff
                    for (int shift = 0; shift < depunc->get_numstates() * 2; shift++) // Test puncturing shifts
                    {
                        int lenp = depunc->depunc_static(ber_soft_buffer, ber_depunc_buffer, TEST_BITS_LENGTH, shift); // Depuncture

                        if (lenp % 2)
                            lenp--;

                        cc_decoder_ber.work(ber_depunc_buffer, ber_decoded_buffer, lenp);      // Decode....
                        cc_encoder_ber.work(ber_decoded_buffer, ber_encoded_buffer, lenp / 2); // ....then reencode for comparison

                        test_bit_len = lenp;

                        d_bers[s][phase][shift] = get_ber(ber_depunc_buffer, ber_encoded_buffer, lenp, depunc->get_berscale()); // Compute BER between initial buffer and re-encoded

                        if (d_bers[s][phase][shift] < d_ber_thresold &&
                            d_bers[s][phase][shift] < d_ber) // Check for a lock
                        {
                            d_ber = d_bers[s][phase][shift]; // Set current BER
                            d_iq_swap = s;                   // Set IQ swap
                            d_state = ST_SYNCED;             // Set the decoder state to SYNCED so we start decoding
                            d_phase = (phase_t)phase;        // Set current phase
                            d_shift = shift;                 // Set current puncturing shift
                            d_invalid = 0;                   // Reset invalid BER count
                            depunc->set_shift(d_shift);

                            memset(soft_buffer, 128, d_buffer_size * 4);
                            memset(depunc_buffer, 128, d_buffer_size * 4);
                        }
                    }
                }
            }
        }

        int out_n = 0; // Output bytes count

        if (d_state == ST_SYNCED) // Decode
        {
            rotate_soft(input, size, PHASE_0, d_iq_swap);
            rotate_soft((int8_t *)input, size, d_phase, false);          // Phase shift
            signed_soft_to_unsigned((int8_t *)input, soft_buffer, size); // Soft convertion

            int sz = depunc->depunc_cont(soft_buffer, depunc_buffer, size); // Depuncturing

            cc_decoder.work(depunc_buffer, output, sz); // Decode entire buffer
            out_n = sz / 2;

            cc_encoder_ber.work(output, ber_encoded_buffer);                     // Re-encoded for a BER check
            d_ber = get_ber(depunc_buffer, ber_encoded_buffer, test_bit_len, 5); // Compute BER

            if (d_ber > d_ber_thresold) // Check current BER
            {
                d_invalid++;
                if (d_invalid > d_max_outsync) // If we get over out max unsynced thresold...
                    d_state = ST_IDLE;         // ...reset the decoder
            }
            else
            {
                d_invalid = 0; // Otherwise, reset current count
            }
        }

        return out_n;
    }

    float Viterbi_Depunc::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;

            for (int s = 0; s < (d_check_iq_swap ? 2 : 1); s++)
                for (phase_t phase : d_phases_to_check)
                    for (int o = 0; o < 12; o++)
                        if (ber > d_bers[s][o][phase])
                            ber = d_bers[s][o][phase];

            return ber;
        }
    }

    int Viterbi_Depunc::getState()
    {
        return d_state;
    }
}
