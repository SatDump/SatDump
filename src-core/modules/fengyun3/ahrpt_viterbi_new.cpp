#include "ahrpt_viterbi_new.h"
#include <cstring>
#include "common/utils.h"
#include "logger.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace fengyun3
{
    DualAHRPTViterbi2::DualAHRPTViterbi2(float ber_threshold, int outsync_after, int buffer_size) : d_ber_thresold(ber_threshold),
                                                                                                    d_outsync_after(outsync_after),
                                                                                                    d_buffer_size(buffer_size),
                                                                                                    d_outsinc(0),
                                                                                                    d_state(ST_IDLE),
                                                                                                    cc_decoder_in_ber(TEST_BITS_LENGTH * 1.5f / 2.0f, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false),
                                                                                                    cc_encoder_in_ber(TEST_BITS_LENGTH * 1.5f / 2.0f, 7, 2, {79, 109}, 0, CC_STREAMING, false),
                                                                                                    depunc_ber(3, 110),
                                                                                                    cc_decoder_in1(8192 * 1.5, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false),
                                                                                                    cc_decoder_in2(8192 * 1.5, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false),
                                                                                                    depunc1(3, 110),
                                                                                                    depunc2(3, 110)
    {
        fixed_soft_packet1 = new uint8_t[buffer_size];
        converted_buffer1 = new uint8_t[buffer_size];
        depunc_buffer1 = new uint8_t[buffer_size * 4];
        output_buffer1 = new uint8_t[buffer_size * 4];

        fixed_soft_packet2 = new uint8_t[buffer_size];
        converted_buffer2 = new uint8_t[buffer_size];
        depunc_buffer2 = new uint8_t[buffer_size * 4];
        output_buffer2 = new uint8_t[buffer_size * 4];
    }

    DualAHRPTViterbi2::~DualAHRPTViterbi2()
    {
        delete[] fixed_soft_packet1;
        delete[] converted_buffer1;
        delete[] depunc_buffer1;
        delete[] output_buffer1;

        delete[] fixed_soft_packet2;
        delete[] converted_buffer2;
        delete[] depunc_buffer2;
        delete[] output_buffer2;
    }

    float DualAHRPTViterbi2::getBER(uint8_t *input)
    {
        char_array_to_uchar((int8_t *)input, d_ber_input_buffer, TEST_BITS_LENGTH);

        depunc_ber.general_work(TEST_BITS_LENGTH, d_ber_input_buffer, d_ber_input_buffer_depunc);

        cc_decoder_in_ber.generic_work(d_ber_input_buffer_depunc, d_ber_decoded_buffer);
        cc_encoder_in_ber.generic_work(d_ber_decoded_buffer, d_ber_encoded_buffer);

        float errors = 0;
        for (int i = 0; i < TEST_BITS_LENGTH * 1.5f; i++)
            if (i % 3 != 2)
                errors += (d_ber_input_buffer_depunc[i] > 0) != (d_ber_encoded_buffer[i] > 0);

        return (errors / ((float)TEST_BITS_LENGTH * 0.5f)) * 4.0f;
    }

    int DualAHRPTViterbi2::work(uint8_t *input1, uint8_t *input2, int size, uint8_t *output1, uint8_t *output2)
    {
        int data_size_out = 0;

        if (d_state == ST_IDLE)
        {
            // Viterbi 1
            {
                // Test without IQ Inversion
                for (int ph = 0; ph < 2; ph++)
                {
                    for (int of = 0; of < 2; of++)
                    {
                        std::memcpy(d_ber_test_buffer, &input1[of * 2], TEST_BITS_LENGTH);
                        phaseShifter.fixPacket(d_ber_test_buffer, TEST_BITS_LENGTH, (sathelper::PhaseShift)ph, false);
                        d_bers[0][of][ph] = getBER(d_ber_test_buffer);
                    }
                }
            }

            // Viterbi 2
            {
                // Test without IQ Inversion
                for (int ph = 0; ph < 2; ph++)
                {
                    for (int of = 0; of < 2; of++)
                    {
                        std::memcpy(d_ber_test_buffer, &input2[of * 2], TEST_BITS_LENGTH);
                        phaseShifter.fixPacket(d_ber_test_buffer, TEST_BITS_LENGTH, (sathelper::PhaseShift)ph, false);
                        d_bers[1][of][ph] = getBER(d_ber_test_buffer);
                    }
                }
            }

            syncedstate[0] = false;
            syncedstate[1] = false;

            for (int p = 0; p < 2; p++)
            {
                for (int o = 0; o < 2; o++)
                {
                    if (d_ber_thresold > d_bers[0][o][p])
                    {
                        d_ber[0] = d_bers[0][o][p];
                        d_iq_inv[0] = 0;
                        d_phase_shift[0] = (sathelper::PhaseShift)p;
                        syncedstate[0] = true;
                        d_skip[0] = o * 2;
                        d_skip_perm[0] = o * 2;
                    }

                    if (d_ber_thresold > d_bers[1][o][p])
                    {
                        d_ber[1] = d_bers[1][o][p];
                        d_iq_inv[1] = 0;
                        d_phase_shift[1] = (sathelper::PhaseShift)p;
                        syncedstate[1] = true;
                        d_skip[1] = o * 2;
                        d_skip_perm[1] = o * 2;
                    }
                }
            }

            if (syncedstate[0] && syncedstate[1])
            {
                d_state = ST_SYNCED;
                depunc1.clear();
                depunc2.clear();
                cc_decoder_in1.clear();
                cc_decoder_in2.clear();
            }
        }

        if (d_state == ST_SYNCED)
        {
            // Decode 1
            {
                std::memcpy(fixed_soft_packet1, &input1[d_skip[0]], size - d_skip[0]);
                phaseShifter.fixPacket(fixed_soft_packet1, size - d_skip[0], d_phase_shift[0], d_iq_inv[0]);

                char_array_to_uchar((int8_t *)fixed_soft_packet1, converted_buffer1, size - d_skip[0]);

                int depunc_out = depunc1.general_work(size - d_skip[0], converted_buffer1, depunc_buffer1);

                int output_size = cc_decoder_in1.continuous_work(depunc_buffer1, depunc_out, output_buffer1);

                data_size_out = repacker.work(output_buffer1, output_size, output1);
            }

            // Decode 2
            {
                std::memcpy(fixed_soft_packet2, &input2[d_skip[1]], size - d_skip[1]);
                phaseShifter.fixPacket(fixed_soft_packet2, size - d_skip[1], d_phase_shift[1], d_iq_inv[1]);

                char_array_to_uchar((int8_t *)fixed_soft_packet2, converted_buffer2, size - d_skip[1]);

                int depunc_out = depunc2.general_work(size - d_skip[1], converted_buffer2, depunc_buffer2);

                int output_size = cc_decoder_in2.continuous_work(depunc_buffer2, depunc_out, output_buffer2);

                data_size_out = repacker.work(output_buffer2, output_size, output2);
            }

            d_skip[1] = 0;
            d_skip[0] = 0;

            // Check BERs
            std::memcpy(fixed_soft_packet1, &input1[d_skip_perm[0]], TEST_BITS_LENGTH);
            phaseShifter.fixPacket(fixed_soft_packet1, TEST_BITS_LENGTH, d_phase_shift[0], d_iq_inv[0]);
            d_ber[0] = getBER(fixed_soft_packet1);

            std::memcpy(fixed_soft_packet2, &input2[d_skip_perm[1]], TEST_BITS_LENGTH);
            phaseShifter.fixPacket(fixed_soft_packet2, TEST_BITS_LENGTH, d_phase_shift[1], d_iq_inv[1]);
            d_ber[1] = getBER(fixed_soft_packet2);

            // Check we're still in sync!
            if (d_ber_thresold < d_ber[0] || d_ber_thresold < d_ber[1])
            {
                d_outsinc++;
                if (d_outsinc >= /*d_outsync_after*/ 10)
                    d_state = ST_IDLE;
            }
            else
            {
                d_outsinc = 0;
            }
        }

        return data_size_out;
    }

    float DualAHRPTViterbi2::ber1()
    {
        if (d_state == ST_SYNCED)
            return d_ber[0];
        else
        {
            float ber = 10;
            for (int p = 0; p < 2; p++)
            {
                for (int o = 0; o < 2; o++)
                {
                    if (ber > d_bers[0][o][p])
                    {
                        ber = d_bers[0][o][p];
                    }
                }
            }
            return ber;
        }
    }

    float DualAHRPTViterbi2::ber2()
    {
        if (d_state == ST_SYNCED)
            return d_ber[1];
        else
        {
            float ber = 10;
            for (int p = 0; p < 2; p++)
            {
                for (int o = 0; o < 2; o++)
                {
                    if (ber > d_bers[1][o][p])
                    {
                        ber = d_bers[1][o][p];
                    }
                }
            }
            return ber;
        }
    }

    int DualAHRPTViterbi2::getState()
    {
        return d_state;
    }
} // namespace npp