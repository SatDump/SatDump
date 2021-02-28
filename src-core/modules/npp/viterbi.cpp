#ifndef __MINGW32__
#ifdef MEMORY_OP_X86
#include "viterbi.h"
#include "modules/common/sathelper/correlator.h"
#include <cstring>
extern "C"
{
#include "modules/common/correct/convolutional/convolutional.h"

    void _convolutional_decode_init(correct_convolutional *conv,
                                    unsigned int min_traceback,
                                    unsigned int traceback_length,
                                    unsigned int renormalize_interval);
    void convolutional_decode_inner(correct_convolutional *sse_conv, unsigned int sets,
                                    const uint8_t *soft);
}
#define ST_IDLE 0
#define ST_SYNCED 1

#define ST_PHASE_0 0
#define ST_PHASE_1 1
#define ST_PHASE_2 2
#define ST_PHASE_3 3
#define ST_PHASE_4 4
#define ST_PHASE_5 5
#define ST_PHASE_6 6
#define ST_PHASE_7 7

#define _VITERBI27_POLYA 0x4F
#define _VITERBI27_POLYB 0x6D

#include "logger.h"

namespace npp
{
    HRDViterbi2::HRDViterbi2(float ber_threshold, int buffer_size) : d_ber_thresold(ber_threshold),
                                                                     d_state(ST_IDLE),
                                                                     d_buffer_size(buffer_size),
                                                                     d_first(true)
    {
        correct_viterbi_ber = correct_convolutional_sse_create(2, 7, new uint16_t[2]{(uint16_t)_VITERBI27_POLYA, (uint16_t)_VITERBI27_POLYB});
        correct_viterbi = correct_convolutional_create(2, 7, new uint16_t[2]{(uint16_t)_VITERBI27_POLYA, (uint16_t)_VITERBI27_POLYB});
        conv = correct_viterbi; //&correct_viterbi->base_conv;
        if (!conv->has_init_decode)
        {
            uint64_t max_error_per_input = conv->rate * soft_max;
            unsigned int renormalize_interval = distance_max / max_error_per_input;
            _convolutional_decode_init(conv, 5 * conv->order, 15 * conv->order, renormalize_interval);
        }

        fixed_soft_packet = new uint8_t[buffer_size + 1024];
        output_buffer = new uint8_t[(buffer_size + 1024) / 16];
    }

    HRDViterbi2::~HRDViterbi2()
    {
        delete[] fixed_soft_packet;
        delete[] output_buffer;
    }

    float HRDViterbi2::getBER(uint8_t *input)
    {
        correct_convolutional_sse_decode_soft(correct_viterbi_ber, input, TEST_BITS_LENGTH, d_ber_decoded_buffer);
        correct_convolutional_sse_encode(correct_viterbi_ber, d_ber_decoded_buffer, TEST_BITS_LENGTH / 16, d_ber_encoded_buffer);

        // Convert to soft bits
        for (int i = 0; i * 8 < TEST_BITS_LENGTH; i++)
        {
            uint8_t d = d_ber_encoded_buffer[i];
            for (int z = 7; z >= 0; z--)
            {
                d_ber_decoded_soft_buffer[i * 8 + (7 - z)] = 0 - ((d & (1 << z)) == 0);
            }
        }

        float errors = 0;
        for (int i = 0; i < TEST_BITS_LENGTH; i++)
        {
            errors += (float)sathelper::Correlator::hardCorrelate(input[i], ~d_ber_decoded_soft_buffer[i]);
        }

        return (errors / ((float)TEST_BITS_LENGTH * 2.0f)) * 10.0f;
    }

    int HRDViterbi2::work(uint8_t *input, size_t size, uint8_t *output)
    {
        int data_size_out = 0;

        switch (d_state)
        {
        case ST_IDLE:
        {
            // Test without IQ Inversion
            for (int ph = 0; ph < 2; ph++)
            {
                std::memcpy(d_ber_test_soft_buffer, input, TEST_BITS_LENGTH);
                phaseShifter.fixPacket(d_ber_test_soft_buffer, TEST_BITS_LENGTH, (sathelper::PhaseShift)ph, false);
                d_bers[0][ph] = getBER(d_ber_test_soft_buffer);
            }

            // Test with IQ Inversion
            for (int ph = 0; ph < 2; ph++)
            {
                std::memcpy(d_ber_test_soft_buffer, input, TEST_BITS_LENGTH);
                phaseShifter.fixPacket(d_ber_test_soft_buffer, TEST_BITS_LENGTH, (sathelper::PhaseShift)ph, true);
                d_bers[1][ph] = getBER(d_ber_test_soft_buffer);
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
                }
            }
        }
        break;

        case ST_SYNCED:
        {
            // Decode
            std::memcpy(fixed_soft_packet, input, size);
            phaseShifter.fixPacket(fixed_soft_packet, size, d_phase_shift, d_iq_inv);

            //conv
            //data_size_out = 1 + (int)correct_convolutional_sse_decode_soft(correct_viterbi, fixed_soft_packet, size, output);
            //logger->info(data_size_out);
            //std::memcpy(output, &output_buffer[1024 / 16], size / 16);
            //std::memcpy(fixed_soft_packet, &fixed_soft_packet[d_buffer_size], 1024);

            //data_size_out = size / 16;

            size_t sets = (size) / conv->rate;
            // XXX fix this vvvvvv
            size_t decoded_len_bytes = size / 16;
            bit_writer_reconfigure(conv->bit_writer, output, decoded_len_bytes);

            //error_buffer_reset(conv->errors);
            //history_buffer_reset(conv->history_buffer);

            if (d_first)
            {
                // no outputs are generated during warmup
                convolutional_decode_warmup(conv, sets, fixed_soft_packet);

                //convolutional_decode_tail(conv, sets, fixed_soft_packet);
                d_first = false;
            }

            convolutional_decode_inner(correct_viterbi, sets, fixed_soft_packet);

            history_buffer_flush(conv->history_buffer, conv->bit_writer);

            data_size_out = bit_writer_length(conv->bit_writer);

            // Check BER
            d_ber = (d_ber + getBER(fixed_soft_packet)) / 2.0f;

            // Check we're still in sync!
            if (d_ber_thresold < d_ber)
            {
                d_state = ST_IDLE;
            }
        }
        break;

        default:
            break;
        }

        return data_size_out;
    }

    float HRDViterbi2::ber()
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

    int HRDViterbi2::getState()
    {
        return d_state;
    }
} // namespace npp
#endif
#endif