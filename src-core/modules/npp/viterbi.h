#pragma once

#ifndef __MINGW32__
#ifdef MEMORY_OP_X86
#define TEST_BITS_LENGTH 1024

extern "C"
{
#include "modules/common/correct/correct.h"
//#ifndef __MINGW32__
//#ifdef MEMORY_OP_X86
#include "modules/common/correct/correct-sse.h"
//#endif
//#endif
}

#include "modules/common/sathelper/packetfixer.h"

namespace npp
{
    class HRDViterbi2
    {
    private:
        int d_buffer_size;
        float d_ber_thresold;
        int d_state;
        bool d_first;

        correct_convolutional_sse *correct_viterbi_ber;
        correct_convolutional *correct_viterbi;
        
        correct_convolutional *conv;

        float d_bers[2][4];
        float d_ber;

        uint8_t d_ber_test_soft_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_decoded_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_decoded_soft_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_encoded_buffer[TEST_BITS_LENGTH];
        float getBER(uint8_t *input);

        sathelper::PhaseShift d_phase_shift;
        bool d_iq_inv;
        sathelper::PacketFixer phaseShifter;

        uint8_t *fixed_soft_packet;
        uint8_t *output_buffer;

    public:
        HRDViterbi2(float ber_threshold, int buffer_size);
        ~HRDViterbi2();

        int work(uint8_t *input, size_t size, uint8_t *output);
        float ber();
        int getState();
    };
} // namespace npp
#endif
#endif