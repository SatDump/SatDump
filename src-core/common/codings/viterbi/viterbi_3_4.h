#pragma once

#define TEST_BITS_LENGTH 2048 // 1024 QPSK symbols

#include "common/codings/rotation.h"
#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/viterbi/cc_encoder.h"

namespace viterbi
{
    class Viterbi3_4
    {
    private:
        // Settings
        const float d_ber_thresold;
        const float d_max_outsync;
        const int d_buffer_size;
        const bool d_fymode;

        // Variables
        int d_state;        // Main decoder state
        phase_t d_phase;    // Phase the decoder locked onto
        bool d_shift;       // Shift the decoder locked onto
        int d_invalid = 0;  // Number of invalid BER tests
        float d_bers[2][2]; // BERs of all branches
        float d_ber;        // Main ber in LOCKED state

        // BER Testing
        CCDecoder cc_decoder_ber;
        CCEncoder cc_encoder_ber;

        // Main decoder
        CCDecoder cc_decoder;

        // BER test buffers
        int8_t ber_test_buffer[TEST_BITS_LENGTH];
        uint8_t ber_soft_buffer[TEST_BITS_LENGTH];
        uint8_t ber_depunc_buffer[TEST_BITS_LENGTH * 2];
        uint8_t ber_decoded_buffer[TEST_BITS_LENGTH * 2];
        uint8_t ber_encoded_buffer[TEST_BITS_LENGTH * 2];

        // Actual decoding buffers
        uint8_t *soft_buffer;
        uint8_t *depunc_buffer;
        uint8_t *output_buffer;

        // Calculate BER between 2 buffers
        float get_ber(uint8_t *raw, uint8_t *rencoded, int len);

        // Depuncture, for rate 3/4 puncturing
        int depuncture(uint8_t *in, uint8_t *out, int size, bool shift);

    public:
        Viterbi3_4(float ber_threshold, int max_outsync, int buffer_size, bool fymode = false);
        ~Viterbi3_4();

        int work(int8_t *input, int size, uint8_t *output);

        bool getshift() { return d_shift; }

        void reset()
        {
            d_state = 0;
        }

        float ber();
        int getState();
    };
}
