#pragma once

#define TEST_BITS_LENGTH 2048 // 1024 QPSK symbols

#include "common/codings/rotation.h"
#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/viterbi/cc_encoder.h"
#include "depunc.h"

/*
DVB-S Oriented, 5 puncturing rates
self-locking streaming viterbi decoder.

QPSK only.

This is meant to automatically detect the
utilized puncturing rate, automatically lock
and start decoding input data.

TODO : Cleanup!!!!
*/
namespace viterbi
{
    enum dvb_rate_t
    {
        RATE_1_2,
        RATE_2_3,
        RATE_3_4,
        RATE_5_6,
        RATE_7_8,
    };

    class Viterbi_DVBS
    {
    private:
        // Settings
        const float d_ber_thresold;
        const float d_max_outsync;
        const int d_buffer_size;
        const std::vector<phase_t> d_phases_to_check;

        // Variables
        int d_state;            // Main decoder state
        dvb_rate_t d_rate;      // Puncturing rate
        phase_t d_phase;        // Phase the decoder locked onto
        int d_shift;            // Shift the decoder locked onto
        int d_invalid = 0;      // Number of invalid BER tests
        float d_bers_12[2][12]; // BERs of all branches, rate 1/2
        float d_bers_23[2][12]; // BERs of all branches, rate 2/3
        float d_bers_34[2][12]; // BERs of all branches, rate 3/4
        float d_bers_56[2][12]; // BERs of all branches, rate 5/6
        float d_bers_78[2][12]; // BERs of all branches, rate 7/8
        float d_ber;            // Main ber in LOCKED state

        // BER Testing
        CCDecoder cc_decoder_ber_12;
        CCEncoder cc_encoder_ber_12;
        CCDecoder cc_decoder_ber_23;
        CCEncoder cc_encoder_ber_23;
        CCDecoder cc_decoder_ber_34;
        CCEncoder cc_encoder_ber_34;
        CCDecoder cc_decoder_ber_56;
        CCEncoder cc_encoder_ber_56;
        CCDecoder cc_decoder_ber_78;
        CCEncoder cc_encoder_ber_78;

        // Main decoder
        CCDecoder cc_decoder_12;
        CCDecoder cc_decoder_23;
        CCDecoder cc_decoder_34;
        CCDecoder cc_decoder_56;
        CCDecoder cc_decoder_78;

        // BER test buffers
        int8_t ber_test_buffer[TEST_BITS_LENGTH];
        uint8_t ber_soft_buffer[TEST_BITS_LENGTH];
        uint8_t ber_depunc_buffer[TEST_BITS_LENGTH * 4];
        uint8_t ber_decoded_buffer[TEST_BITS_LENGTH * 4];
        uint8_t ber_encoded_buffer[TEST_BITS_LENGTH * 4];

        // Actual decoding buffers
        uint8_t *soft_buffer;
        uint8_t *depunc_buffer;
        uint8_t *output_buffer;

        // Calculate BER between 2 buffers
        float get_ber(uint8_t *raw, uint8_t *rencoded, int len, float ratio);

        Depunc23 depunc_32;
        Depunc56 depunc_56;

        int depuncture_34(uint8_t *in, uint8_t *out, int size, bool shift)
        {
            int oo = 0;

            for (int i = 0; i < size / 2; i++)
            {
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
            }

            return oo;
        }

        int depuncture_78(uint8_t *in, uint8_t *out, int size, int shift)
        {
            int oo = 0;

            for (int i = 0; i < size / 2; i++)
            {
                if ((i + shift) % 4 == 0)
                {
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = in[i * 2 + 1];
                }
                else if ((i + shift) % 4 == 1)
                {
                    out[oo++] = 128;
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = 128;
                    out[oo++] = in[i * 2 + 1];
                }
                else if ((i + shift) % 4 == 2)
                {
                    out[oo++] = 128;
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = in[i * 2 + 1];
                    out[oo++] = 128;
                }
                else if ((i + shift) % 4 == 3)
                {
                    out[oo++] = 128;
                    out[oo++] = in[i * 2 + 0];
                    out[oo++] = in[i * 2 + 1];
                    out[oo++] = 128;
                }
            }

            return oo;
        }

    public:
        Viterbi_DVBS(float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases = {PHASE_0, PHASE_90, PHASE_180, PHASE_270});
        ~Viterbi_DVBS();

        int work(int8_t *input, int size, uint8_t *output);

        bool getshift() { return d_shift; }

        float ber();
        int getState();
        dvb_rate_t rate() { return d_rate; }
    };
}
