#pragma once

#define TEST_BITS_LENGTH 1024

#include "libs/sathelper/packetfixer.h"
#include "common/codings/viterbi/cc_decoder_impl.h"
#include "common/codings/viterbi/cc_encoder_impl.h"
#include "common/codings/viterbi/depuncture_bb_impl.h"
#include "common/repack_bits_byte.h"

namespace fengyun
{
    class MPTViterbi2
    {
    private:
        // Settings
        const int d_buffer_size;
        const int d_outsync_after;
        const float d_ber_thresold;

        // Variables
        int d_outsinc;
        int d_state;
        bool d_first;
        float d_bers[2][4];
        float d_ber;

        // BER Decoders
        fec::code::cc_decoder_impl cc_decoder_in_ber;
        fec::code::cc_encoder_impl cc_encoder_in_ber;

        // BER Buffers
        uint8_t d_ber_test_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_input_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_decoded_buffer[TEST_BITS_LENGTH / 2];
        uint8_t d_ber_encoded_buffer[TEST_BITS_LENGTH];

        // Current phase status
        sathelper::PhaseShift d_phase_shift;
        bool d_iq_inv;
        sathelper::PacketFixer phaseShifter;

        // Work buffers
        uint8_t *fixed_soft_packet;
        float *convert_buffer;
        uint8_t *viterbi_in;
        uint8_t *output_buffer;

        // Main decoder
        fec::code::cc_decoder_impl cc_decoder_in;

        // Repacker
        RepackBitsByte repacker;

        float getBER(uint8_t *input);

        void char_array_to_uchar(const char *in, unsigned char *out, int nsamples)
        {
            for (int i = 0; i < nsamples; i++)
            {
                long int r = (long int)rint((float)in[i] * 127.0);
                if (r < 0)
                    r = 0;
                else if (r > 255)
                    r = 255;
                out[i] = r;
            }
        }

    public:
        MPTViterbi2(float ber_threshold, int outsync_after, int buffer_size);
        ~MPTViterbi2();

        int work(uint8_t *input, size_t size, uint8_t *output);
        float ber();
        int getState();
    };
} // namespace npp