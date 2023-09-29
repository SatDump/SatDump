#pragma once

#define TEST_BITS_LENGTH 2048 // 1024 QPSK symbols

#include "common/codings/rotation.h"
#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/viterbi/cc_encoder.h"
#include "depunc.h"
#include <memory>

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
    class Viterbi_Depunc
    {
    private:
        std::shared_ptr<puncturing::GenericDepunc> depunc;

        // Settings
        const float d_ber_thresold;
        const float d_max_outsync;
        const bool d_check_iq_swap;
        const int d_buffer_size;
        const std::vector<phase_t> d_phases_to_check;

        // Variables
        int d_state;            // Main decoder state
        bool d_iq_swap;         // IQ Swap the decoder locked onto
        phase_t d_phase;        // Phase the decoder locked onto
        int d_shift;            // Shift the decoder locked onto
        int d_invalid = 0;      // Number of invalid BER tests
        float d_bers[2][12][2]; // BERs of all branches
        float d_ber;            // Main ber in LOCKED state

        // BER Testing
        CCDecoder cc_decoder_ber;
        CCEncoder cc_encoder_ber;

        // Main decoder
        CCDecoder cc_decoder;

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

        int test_bit_len = 0;

    public:
        Viterbi_Depunc(std::shared_ptr<puncturing::GenericDepunc> depunc, float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases = {PHASE_0, PHASE_90, PHASE_180, PHASE_270}, bool check_iq_swap = false);
        ~Viterbi_Depunc();

        int work(int8_t *input, int size, uint8_t *output);

        bool getshift() { return d_shift; }

        float ber();
        int getState();
    };
}
