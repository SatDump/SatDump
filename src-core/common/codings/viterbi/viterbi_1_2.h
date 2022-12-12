#pragma once

#define TEST_BITS_LENGTH 2048 // 1024 QPSK symbols

#include "common/codings/rotation.h"
#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/viterbi/cc_encoder.h"

namespace viterbi
{
    class Viterbi1_2
    {
    private:
        // Settings
        const float d_ber_thresold;
        const float d_max_outsync;
        const bool d_check_iq_swap;
        const int d_buffer_size;
        const std::vector<phase_t> d_phases_to_check;

        // Variables
        int d_state;           // Main decoder state
        bool d_iq_swap;        // IQ Swap the decoder locked onto
        phase_t d_phase;       // Phase the decoder locked onto
        bool d_shift;          // Shift the decoder locked onto
        int d_invalid = 0;     // Number of invalid BER tests
        float d_bers[2][4][2]; // BERs of all branches
        float d_ber;           // Main ber in LOCKED state

        // BER Testing
        CCDecoder cc_decoder_ber;
        CCEncoder cc_encoder_ber;

        // Main decoder
        CCDecoder cc_decoder;

        // BER test buffers
        int8_t ber_test_buffer[TEST_BITS_LENGTH];
        uint8_t ber_soft_buffer[TEST_BITS_LENGTH];
        uint8_t ber_decoded_buffer[TEST_BITS_LENGTH];
        uint8_t ber_encoded_buffer[TEST_BITS_LENGTH];

        // Actual decoding buffers
        uint8_t *soft_buffer;
        uint8_t *output_buffer;

        // Calculate BER between 2 buffers
        float get_ber(uint8_t *raw, uint8_t *rencoded, int len);

    public:
        Viterbi1_2(float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases = {PHASE_0, PHASE_90, PHASE_180, PHASE_270}, bool check_iq_swap = false);
        ~Viterbi1_2();

        int work(int8_t *input, int size, uint8_t *output);

        bool getshift() { return d_shift; }

        float ber();
        int getState();
    };
}
