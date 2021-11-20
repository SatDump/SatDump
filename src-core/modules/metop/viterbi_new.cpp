#include "viterbi_new.h"
#include <cstring>
#include "common/utils.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace metop
{
    MetopViterbi2::MetopViterbi2(float ber_threshold, int outsync_after, int buffer_size) : d_ber_thresold(ber_threshold),
                                                                                            d_outsync_after(outsync_after),
                                                                                            d_buffer_size(buffer_size),
                                                                                            d_outsinc(0),
                                                                                            d_state(ST_IDLE),
                                                                                            d_first(true),
                                                                                            depunc_ber(3, 110),
                                                                                            cc_decoder_in_ber(TEST_BITS_LENGTH * 1.5f / 2.0f, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false),
                                                                                            cc_encoder_in_ber(TEST_BITS_LENGTH * 1.5f / 2.0f, 7, 2, {79, 109}, 0, CC_STREAMING, false),
                                                                                            cc_decoder_in(8192 * 1.5, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false),
                                                                                            depunc(3, 110)
    {
        fixed_soft_packet = new uint8_t[buffer_size];
        converted_buffer = new uint8_t[buffer_size];
        reorg_buffer = new uint8_t[buffer_size * 2];
        depunc_buffer = new uint8_t[buffer_size * 4];
        output_buffer = new uint8_t[buffer_size * 4];
        d_skip = 0;
        d_skip_perm = 0;
    }

    MetopViterbi2::~MetopViterbi2()
    {
        delete[] fixed_soft_packet;
        delete[] converted_buffer;
        delete[] reorg_buffer;
        delete[] depunc_buffer;
        delete[] output_buffer;
    }

    float MetopViterbi2::getBER(uint8_t *input)
    {
        char_array_to_uchar((int8_t *)input, d_ber_input_buffer, TEST_BITS_LENGTH);

        for (int i = 0; i < TEST_BITS_LENGTH / 4; i++)
        {
            d_ber_input_reorg_buffer[i * 4 + 0] = d_ber_input_buffer[i * 4 + 0];
            d_ber_input_reorg_buffer[i * 4 + 1] = d_ber_input_buffer[i * 4 + 1];
            d_ber_input_reorg_buffer[i * 4 + 2] = d_ber_input_buffer[i * 4 + 3];
            d_ber_input_reorg_buffer[i * 4 + 3] = d_ber_input_buffer[i * 4 + 2];
        }

        depunc_ber.general_work(TEST_BITS_LENGTH, d_ber_input_reorg_buffer, d_ber_input_buffer_depunc);

        cc_decoder_in_ber.generic_work(d_ber_input_buffer_depunc, d_ber_decoded_buffer);
        cc_encoder_in_ber.generic_work(d_ber_decoded_buffer, d_ber_encoded_buffer);

        float errors = 0;
        for (int i = 0; i < TEST_BITS_LENGTH * 1.5f; i++)
            if (i % 3 != 2)
                errors += (d_ber_input_buffer_depunc[i] > 0) != (d_ber_encoded_buffer[i] > 0);

        return (errors / ((float)TEST_BITS_LENGTH * 1.0f)) * 4.0f;
    }

    int MetopViterbi2::work(uint8_t *input, int size, uint8_t *output)
    {
        int data_size_out = 0;

        if (d_state == ST_IDLE)
        {
            // Test without IQ Inversion
            for (int ph = 0; ph < 2; ph++)
            {
                for (int of = 0; of < 2; of++)
                {
                    std::memcpy(d_ber_test_buffer, &input[of * 2], TEST_BITS_LENGTH);
                    rotate_soft((int8_t *)d_ber_test_buffer, TEST_BITS_LENGTH, (phase_t)ph, true);
                    d_bers[of][ph] = getBER(d_ber_test_buffer);
                }
            }

            for (int p = 0; p < 2; p++)
            {
                for (int o = 0; o < 2; o++)
                {
                    if (d_ber_thresold > d_bers[o][p])
                    {
                        d_ber = d_bers[o][p];
                        d_iq_inv = 0;
                        d_phase_shift = (phase_t)(p + 1);
                        d_state = ST_SYNCED;
                        d_skip = o * 2;
                        d_skip_perm = o * 2;
                        reorg.clear();
                    }
                }
            }
        }

        if (d_state == ST_SYNCED)
        {
            // Decode
            std::memcpy(fixed_soft_packet, &input[d_skip], size - d_skip);
            rotate_soft((int8_t *)fixed_soft_packet, size - d_skip, d_phase_shift, d_iq_inv);

            char_array_to_uchar((int8_t *)fixed_soft_packet, converted_buffer, size - d_skip);

            int reorg_out = reorg.work(converted_buffer, size - d_skip, reorg_buffer);

            int depunc_out = depunc.general_work(reorg_out, reorg_buffer, depunc_buffer);

            int output_size = cc_decoder_in.continuous_work(depunc_buffer, depunc_out, output_buffer);

            data_size_out = repacker.work(output_buffer, output_size, output);

            d_skip = 0;

            // Check BER
            std::memcpy(fixed_soft_packet, &input[d_skip_perm], TEST_BITS_LENGTH);
            rotate_soft((int8_t *)fixed_soft_packet, size - d_skip, d_phase_shift, d_iq_inv);
            d_ber = getBER(fixed_soft_packet);

            // Check we're still in sync!
            if (d_ber_thresold < d_ber)
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

    float MetopViterbi2::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;
            for (int p = 0; p < 2; p++)
            {
                for (int o = 0; o < 2; o++)
                {
                    if (ber > d_bers[o][p])
                    {
                        ber = d_bers[o][p];
                    }
                }
            }
            return ber;
        }
    }

    int MetopViterbi2::getState()
    {
        return d_state;
    }
} // namespace npp