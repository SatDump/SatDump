#include "viterbi_all.h"
#include <cstring>
#include "common/utils.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace viterbi
{
    Viterbi_DVBS::Viterbi_DVBS(float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases)
        : d_ber_thresold(ber_threshold),
          d_max_outsync(max_outsync),
          d_buffer_size(buffer_size),
          d_phases_to_check(phases),
          d_state(ST_IDLE),

          cc_decoder_ber_12(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}),
          cc_encoder_ber_12(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}),
          cc_decoder_ber_23(TEST_BITS_LENGTH * 1.334 / 2, 7, 2, {79, 109}),
          cc_encoder_ber_23(TEST_BITS_LENGTH * 1.334 / 2, 7, 2, {79, 109}),
          cc_decoder_ber_34(TEST_BITS_LENGTH * 1.5 / 2, 7, 2, {79, 109}),
          cc_encoder_ber_34(TEST_BITS_LENGTH * 1.5 / 2, 7, 2, {79, 109}),
          cc_decoder_ber_56(TEST_BITS_LENGTH * 1.66 / 2, 7, 2, {79, 109}),
          cc_encoder_ber_56(TEST_BITS_LENGTH * 1.66 / 2, 7, 2, {79, 109}),
          cc_decoder_ber_78(TEST_BITS_LENGTH * 1.75 / 2, 7, 2, {79, 109}),
          cc_encoder_ber_78(TEST_BITS_LENGTH * 1.75 / 2, 7, 2, {79, 109}),

          cc_decoder_12(buffer_size / 2, 7, 2, {79, 109}),
          cc_decoder_23(10924 / 2, 7, 2, {79, 109}),
          cc_decoder_34(buffer_size * 1.5 / 2, 7, 2, {79, 109}),
          cc_decoder_56(buffer_size * 1.66 / 2, 7, 2, {79, 109}),
          cc_decoder_78(buffer_size * 1.75 / 2, 7, 2, {79, 109})

    {
        soft_buffer = new uint8_t[d_buffer_size * 4];
        depunc_buffer = new uint8_t[d_buffer_size * 4];
        output_buffer = new uint8_t[d_buffer_size * 4];

        for (int i = 0; i < 12; i++)
        {
            for (int y = 0; y < 2; y++)
            {
                d_bers_12[y][i] = 10;
                d_bers_23[y][i] = 10;
                d_bers_34[y][i] = 10;
                d_bers_56[y][i] = 10;
                d_bers_78[y][i] = 10;
            }
        }
    }

    Viterbi_DVBS::~Viterbi_DVBS()
    {
        delete[] soft_buffer;
        delete[] depunc_buffer;
        delete[] output_buffer;
    }

    float Viterbi_DVBS::get_ber(uint8_t *raw, uint8_t *rencoded, int len, float ratio)
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

    int Viterbi_DVBS::work(int8_t *input, int size, uint8_t *output)
    {
        if (d_state == ST_IDLE) // Search for a lock
        {
            d_ber = 10;
            for (phase_t phase : d_phases_to_check)
            {
                memcpy(ber_test_buffer, input, TEST_BITS_LENGTH);                            // Copy over small buffer
                rotate_soft(ber_test_buffer, TEST_BITS_LENGTH, phase, false);                // Phase shift
                signed_soft_to_unsigned(ber_test_buffer, ber_soft_buffer, TEST_BITS_LENGTH); // Convert to softs for the viterbi

                // Rate 1/2
                for (int shift = 0; shift < 2; shift++) // Test 2 puncturing shifts
                {
                    cc_decoder_ber_12.work(ber_soft_buffer + shift, ber_decoded_buffer); // Decode....
                    cc_encoder_ber_12.work(ber_decoded_buffer, ber_encoded_buffer);      // ....then reencode for comparison

                    d_bers_12[phase][shift] = get_ber(ber_soft_buffer + shift, ber_encoded_buffer, TEST_BITS_LENGTH, 2.5); // Compute BER between initial buffer and re-encoded

                    if (d_bers_12[phase][shift] < d_ber_thresold) // Check for a lock
                    {
                        d_ber = d_bers_12[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;             // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;        // Set current phase
                        d_shift = shift;                 // Set current puncturing shift
                        d_invalid = 0;                   // Reset invalid BER count
                        d_rate = RATE_1_2;               // Set rate

                        memset(soft_buffer, 128, d_buffer_size * 4);
                        memset(depunc_buffer, 128, d_buffer_size * 4);
                    }
                }

                // Rate 2/3
                for (int shift = 0; shift < 6; shift++) // Test 3 puncturing shifts
                {
                    depunc_32.depunc_static(ber_soft_buffer, ber_depunc_buffer, TEST_BITS_LENGTH, shift); // Depuncture

                    cc_decoder_ber_23.work(ber_depunc_buffer, ber_decoded_buffer);  // Decode....
                    cc_encoder_ber_23.work(ber_decoded_buffer, ber_encoded_buffer); // ....then reencode for comparison

                    d_bers_23[phase][shift] = get_ber(ber_depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.25, 3.5); // Compute BER between initial buffer and re-encoded

                    if (d_bers_23[phase][shift] < d_ber_thresold) // Check for a lock
                    {
                        d_ber = d_bers_23[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;             // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;        // Set current phase
                        d_shift = shift;                 // Set current puncturing shift
                        d_invalid = 0;                   // Reset invalid BER count
                        d_rate = RATE_2_3;               // Set rate
                        depunc_32.set_shift(d_shift);

                        memset(soft_buffer, 128, d_buffer_size * 4);
                        memset(depunc_buffer, 128, d_buffer_size * 4);
                    }
                }

                // Rate 3/4
                for (int shift = 0; shift < 2; shift++) // Test 2 puncturing shifts
                {
                    depuncture_34(ber_soft_buffer, ber_depunc_buffer, TEST_BITS_LENGTH, shift); // Depuncture

                    cc_decoder_ber_34.work(ber_depunc_buffer, ber_decoded_buffer);  // Decode....
                    cc_encoder_ber_34.work(ber_decoded_buffer, ber_encoded_buffer); // ....then reencode for comparison

                    d_bers_34[phase][shift] = get_ber(ber_depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.5, 5); // Compute BER between initial buffer and re-encoded

                    if (d_bers_34[phase][shift] < d_ber_thresold) // Check for a lock
                    {
                        d_ber = d_bers_34[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;             // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;        // Set current phase
                        d_shift = shift;                 // Set current puncturing shift
                        d_invalid = 0;                   // Reset invalid BER count
                        d_rate = RATE_3_4;               // Set rate

                        memset(soft_buffer, 128, d_buffer_size * 4);
                        memset(depunc_buffer, 128, d_buffer_size * 4);
                    }
                }

                // Rate 5/6
                for (int shift = 0; shift < 12; shift++) // Test 3 puncturing shifts
                {
                    depunc_56.depunc_static(ber_soft_buffer, ber_depunc_buffer, TEST_BITS_LENGTH, shift); // Depuncture

                    cc_decoder_ber_56.work(ber_depunc_buffer, ber_decoded_buffer);  // Decode....
                    cc_encoder_ber_56.work(ber_decoded_buffer, ber_encoded_buffer); // ....then reencode for comparison

                    d_bers_56[phase][shift] = get_ber(ber_depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.66, 8); // Compute BER between initial buffer and re-encoded

                    if (d_bers_56[phase][shift] < d_ber_thresold) // Check for a lock
                    {
                        d_ber = d_bers_56[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;             // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;        // Set current phase
                        d_shift = shift;                 // Set current puncturing shift
                        d_invalid = 0;                   // Reset invalid BER count
                        d_rate = RATE_5_6;               // Set rate
                        depunc_56.set_shift(d_shift);

                        memset(soft_buffer, 128, d_buffer_size * 4);
                        memset(depunc_buffer, 128, d_buffer_size * 4);
                    }
                }

                // Rate 7/8
                for (int shift = 0; shift < 4; shift++) // Test 3 puncturing shifts
                {
                    depuncture_78(ber_soft_buffer, ber_depunc_buffer, TEST_BITS_LENGTH, shift); // Depuncture

                    cc_decoder_ber_78.work(ber_depunc_buffer, ber_decoded_buffer);  // Decode....
                    cc_encoder_ber_78.work(ber_decoded_buffer, ber_encoded_buffer); // ....then reencode for comparison

                    d_bers_78[phase][shift] = get_ber(ber_depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.75, 10); // Compute BER between initial buffer and re-encoded

                    if (d_bers_78[phase][shift] < d_ber_thresold) // Check for a lock
                    {
                        d_ber = d_bers_78[phase][shift]; // Set current BER
                        d_state = ST_SYNCED;             // Set the decoder state to SYNCED so we start decoding
                        d_phase = (phase_t)phase;        // Set current phase
                        d_shift = shift;                 // Set current puncturing shift
                        d_invalid = 0;                   // Reset invalid BER count
                        d_rate = RATE_7_8;               // Set rate

                        memset(soft_buffer, 128, d_buffer_size * 4);
                        memset(depunc_buffer, 128, d_buffer_size * 4);
                    }
                }
            }
        }

        int out_n = 0; // Output bytes count

        if (d_state == ST_SYNCED) // Decode
        {
            rotate_soft((int8_t *)input, size, d_phase, false);          // Phase shift
            signed_soft_to_unsigned((int8_t *)input, soft_buffer, size); // Soft convertion

            if (d_rate == RATE_1_2)
            {
                cc_decoder_12.work(soft_buffer + d_shift, output, size); // Decode entire buffer
                out_n = size / 2;

                cc_encoder_ber_12.work(output, ber_encoded_buffer);                                // Re-encoded for a BER check
                d_ber = get_ber(soft_buffer + d_shift, ber_encoded_buffer, TEST_BITS_LENGTH, 2.5); // Compute BER
            }
            else if (d_rate == RATE_2_3)
            {
                int sz = depunc_32.depunc_cont(soft_buffer, depunc_buffer, size); // Depuncturing

                cc_decoder_23.work(depunc_buffer, output, sz); // Decode entire buffer
                out_n = sz / 2;

                cc_encoder_ber_23.work(output, ber_encoded_buffer);                               // Re-encoded for a BER check
                d_ber = get_ber(depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.25, 3.5); // Compute BER
            }
            else if (d_rate == RATE_3_4)
            {
                int sz = depuncture_34(soft_buffer, depunc_buffer, size, d_shift); // Depuncturing

                cc_decoder_34.work(depunc_buffer, output, sz); // Decode entire buffer
                out_n = sz / 2;

                cc_encoder_ber_34.work(output, ber_encoded_buffer);                            // Re-encoded for a BER check
                d_ber = get_ber(depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.5, 5); // Compute BER
            }
            else if (d_rate == RATE_5_6)
            {
                int sz = depunc_56.depunc_cont(soft_buffer, depunc_buffer, size); // Depuncturing

                cc_decoder_56.work(depunc_buffer, output, sz); // Decode entire buffer
                out_n = sz / 2;

                cc_encoder_ber_56.work(output, ber_encoded_buffer);                             // Re-encoded for a BER check
                d_ber = get_ber(depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.66, 8); // Compute BER
            }
            else if (d_rate == RATE_7_8)
            {
                int sz = depuncture_78(soft_buffer, depunc_buffer, size, d_shift); // Depuncturing

                cc_decoder_78.work(depunc_buffer, output, sz); // Decode entire buffer
                out_n = sz / 2;

                cc_encoder_ber_78.work(output, ber_encoded_buffer);                              // Re-encoded for a BER check
                d_ber = get_ber(depunc_buffer, ber_encoded_buffer, TEST_BITS_LENGTH * 1.75, 10); // Compute BER
            }

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

    float Viterbi_DVBS::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;

            for (phase_t phase : d_phases_to_check)
                for (int o = 0; o < 2; o++)
                    if (ber > d_bers_12[phase][o])
                        ber = d_bers_12[phase][o];

            for (phase_t phase : d_phases_to_check)
                for (int o = 0; o < 6; o++)
                    if (ber > d_bers_23[phase][o])
                        ber = d_bers_23[phase][o];

            for (phase_t phase : d_phases_to_check)
                for (int o = 0; o < 2; o++)
                    if (ber > d_bers_34[phase][o])
                        ber = d_bers_34[phase][o];

            for (phase_t phase : d_phases_to_check)
                for (int o = 0; o < 12; o++)
                    if (ber > d_bers_56[phase][o])
                        ber = d_bers_56[phase][o];

            for (phase_t phase : d_phases_to_check)
                for (int o = 0; o < 4; o++)
                    if (ber > d_bers_78[phase][o])
                        ber = d_bers_78[phase][o];

            return ber;
        }
    }

    int Viterbi_DVBS::getState()
    {
        return d_state;
    }
}
