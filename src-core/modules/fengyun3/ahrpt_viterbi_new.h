#pragma once

#define TEST_BITS_LENGTH 1024

#include "libs/sathelper/packetfixer.h"
#include "common/codings/viterbi/cc_decoder_impl.h"
#include "common/codings/viterbi/cc_encoder_impl.h"
#include "common/codings/viterbi/depuncture_bb_impl.h"
#include "common/repack_bits_byte.h"

namespace fengyun3
{
    class DualAHRPTViterbi2
    {
    private:
        // Settings
        const float d_ber_thresold;
        const int d_outsync_after;
        const int d_buffer_size;

        // Variables
        int d_outsinc;
        int d_state;
        bool syncedstate[2];
        float d_bers[2][4][2];
        float d_ber[2];

        // BER Decoders
        fec::code::cc_decoder_impl cc_decoder_in_ber;
        fec::code::cc_encoder_impl cc_encoder_in_ber;
        fec::depuncture_bb_impl depunc_ber;

        // BER Buffers
        uint8_t d_ber_test_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_input_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_input_buffer_depunc[TEST_BITS_LENGTH * 2]; // 1.5
        uint8_t d_ber_decoded_buffer[TEST_BITS_LENGTH * 2];
        uint8_t d_ber_encoded_buffer[TEST_BITS_LENGTH * 2]; // 1.5

        // Current phase status
        sathelper::PhaseShift d_phase_shift[2];
        bool d_iq_inv[2];
        int d_skip[2], d_skip_perm[2];
        sathelper::PacketFixer phaseShifter;

        // Work buffers
        uint8_t *fixed_soft_packet1;
        uint8_t *converted_buffer1;
        uint8_t *depunc_buffer1;
        uint8_t *output_buffer1;
        uint8_t *fixed_soft_packet2;
        uint8_t *converted_buffer2;
        uint8_t *depunc_buffer2;
        uint8_t *output_buffer2;

        // Main decoder
        fec::code::cc_decoder_impl cc_decoder_in1, cc_decoder_in2;
        fec::depuncture_bb_impl depunc1, depunc2;

        // Repacker
        RepackBitsByte repacker;

        float getBER(uint8_t *input);

    public:
        DualAHRPTViterbi2(float ber_threshold, int outsync_after, int buffer_size);
        ~DualAHRPTViterbi2();

        int work(uint8_t *input1, uint8_t *input2, int size, uint8_t *output1, uint8_t *output2);
        float ber1();
        float ber2();
        int getState();
    };
} // namespace npp