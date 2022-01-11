#include "viterbi_1_2.h"
#include <cstring>
#include "common/utils.h"

#include <iostream>

#define ST_IDLE 0
#define ST_SYNCED 1

namespace viterbi
{
    Viterbi1_2::Viterbi1_2(float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases) : d_ber_thresold(ber_threshold),
                                                                                                                 d_max_outsync(max_outsync),
                                                                                                                 d_buffer_size(buffer_size),
                                                                                                                 d_phases_to_check(phases),
                                                                                                                 d_state(ST_IDLE),
                                                                                                                 cc_decoder_ber(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}),
                                                                                                                 cc_encoder_ber(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}),
                                                                                                                 cc_decoder(buffer_size / 2, 7, 2, {79, 109})

    {
        soft_buffer = new uint8_t[d_buffer_size * 2];
        output_buffer = new uint8_t[d_buffer_size * 2];
    }

    Viterbi1_2::~Viterbi1_2()
    {
        delete[] soft_buffer;
        delete[] output_buffer;
    }

    float Viterbi1_2::get_ber(uint8_t *raw, uint8_t *rencoded, int len)
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

        return (errors / total) * 2.5;
    }

    int Viterbi1_2::work(int8_t *input, int size, uint8_t *output)
    {
        if (d_state == ST_IDLE) // Search for a lock
        {
            for (phase_t phase : d_phases_to_check)
            {
                memcpy(ber_test_buffer, input, TEST_BITS_LENGTH);                            // Copy over small buffer
                rotate_soft(ber_test_buffer, TEST_BITS_LENGTH, phase, false);                // Phase shift
                signed_soft_to_unsigned(ber_test_buffer, ber_soft_buffer, TEST_BITS_LENGTH); // Convert to softs for the viterbi

                for (int shift = 0; shift < 2; shift++) // Test 2 puncturing shifts
                {
                    cc_decoder_ber.generic_work(ber_soft_buffer + shift, ber_decoded_buffer); // Decode....
                    cc_encoder_ber.generic_work(ber_decoded_buffer, ber_encoded_buffer);      // ....then reencode for comparison

                    d_bers[phase][shift] = get_ber(ber_soft_buffer + shift, ber_encoded_buffer, TEST_BITS_LENGTH); // Compute BER between initial buffer and re-encoded

                    if (d_bers[phase][shift] < d_ber_thresold) // Check for a lock
                    {
                        d_ber = d_bers[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;          // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;     // Set current phase
                        d_shift = shift;              // Set current puncturing shift
                        d_invalid = 0;                // Reset invalid BER count
                    }
                }
            }
        }

        int out_n = 0; // Output bytes count

        if (d_state == ST_SYNCED) // Decode
        {
            rotate_soft((int8_t *)input, size, d_phase, false);          // Phase shift
            signed_soft_to_unsigned((int8_t *)input, soft_buffer, size); // Soft convertion

            cc_decoder.generic_work(soft_buffer + d_shift, output); // Decode entire buffer
            out_n = size / 2;

            cc_encoder_ber.generic_work(output, ber_encoded_buffer);                      // Re-encoded for a BER check
            d_ber = get_ber(soft_buffer + d_shift, ber_encoded_buffer, TEST_BITS_LENGTH); // Compute BER

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

    float Viterbi1_2::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;
            for (int p = 0; p < d_phases_to_check.size(); p++)
            {
                for (int o = 0; o < 2; o++)
                {
                    if (ber > d_bers[p][o])
                    {
                        ber = d_bers[p][o];
                    }
                }
            }
            return ber;
        }
    }

    int Viterbi1_2::getState()
    {
        return d_state;
    }
} // namespace npp