#include "mpt_viterbi_new.h"
#include <cstring>
#include "logger.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace fengyun3
{
    MPTViterbi2::MPTViterbi2(float ber_threshold, int outsync_after, int buffer_size) : d_buffer_size(buffer_size),
                                                                                        d_outsync_after(outsync_after),
                                                                                        d_ber_thresold(ber_threshold),
                                                                                        d_outsinc(0),
                                                                                        d_state(ST_IDLE),
                                                                                        d_first(true),
                                                                                        cc_decoder_in_ber(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false),
                                                                                        cc_encoder_in_ber(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}, 0, CC_STREAMING, false),
                                                                                        cc_decoder_in(8192, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false)
    {
        fixed_soft_packet = new uint8_t[buffer_size];
        viterbi_in = new uint8_t[buffer_size];
        convert_buffer = new float[buffer_size];
        output_buffer = new uint8_t[buffer_size * 2];
        d_ber = 0;
    }

    MPTViterbi2::~MPTViterbi2()
    {
        delete[] fixed_soft_packet;
        delete[] viterbi_in;
        delete[] convert_buffer;
        delete[] output_buffer;
    }

    float MPTViterbi2::getBER(uint8_t *input)
    {
        char_array_to_uchar((char *)input, d_ber_input_buffer, TEST_BITS_LENGTH);
        cc_decoder_in_ber.generic_work(d_ber_input_buffer, d_ber_decoded_buffer);
        cc_encoder_in_ber.generic_work(d_ber_decoded_buffer, d_ber_encoded_buffer);

        float errors = 0;
        for (int i = 0; i < TEST_BITS_LENGTH; i++)
            errors += (d_ber_input_buffer[i] > 0) != (d_ber_encoded_buffer[i] > 0);

        return (errors / ((float)TEST_BITS_LENGTH * 2.0f)) * 4.0f;
    }

    int MPTViterbi2::work(uint8_t *input, size_t size, uint8_t *output)
    {
        int data_size_out = 0;

        if (d_state == ST_IDLE)
        {
            // Test without IQ Inversion
            for (int sh = 0; sh < 2; sh++)
            {
                for (int ph = 0; ph < 2; ph++)
                {
                    std::memcpy(d_ber_test_buffer, input, TEST_BITS_LENGTH);
                    phaseShifter.fixPacket(d_ber_test_buffer, TEST_BITS_LENGTH, (sathelper::PhaseShift)ph, sh);
                    d_bers[sh][ph] = getBER(d_ber_test_buffer);
                }
            }

            for (int s = 0; s < 2; s++)
            {
                for (int p = 0; p < 2; p++)
                {
                    if (d_ber_thresold > d_bers[s][p])
                    {
                        d_ber = d_bers[s][p];
                        d_iq_inv = s;
                        d_phase_shift = (sathelper::PhaseShift)p;
                        d_state = ST_SYNCED;
                    }
                    //logger->info(std::to_string(s) + "," + std::to_string(p) + " - " + std::to_string(d_bers[s][p]));
                }
            }
        }

        if (d_state == ST_SYNCED)
        {
            // Decode
            std::memcpy(fixed_soft_packet, input, size);
            phaseShifter.fixPacket(fixed_soft_packet, size, d_phase_shift, d_iq_inv);

            char_array_to_uchar((char *)fixed_soft_packet, viterbi_in, size);

            int output_size = cc_decoder_in.continuous_work(viterbi_in, size, output_buffer);

            data_size_out = repacker.work(output_buffer, output_size, output);

            // Check BER
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

    float MPTViterbi2::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;
            for (int s = 0; s < 2; s++)
            {
                for (int p = 0; p < 2; p++)
                {
                    if (ber > d_bers[s][p])
                    {
                        ber = d_bers[s][p];
                    }
                }
            }
            return ber;
        }
    }

    int MPTViterbi2::getState()
    {
        return d_state;
    }
} // namespace npp