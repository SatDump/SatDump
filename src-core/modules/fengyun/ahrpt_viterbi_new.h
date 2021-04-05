#pragma once

#define TEST_BITS_LENGTH 1024

#include "modules/common/sathelper/packetfixer.h"
#include "modules/common/viterbi/cc_decoder_impl.h"
#include "modules/common/viterbi/cc_encoder_impl.h"
#include "modules/common/viterbi/depuncture_bb_impl.h"
#include "modules/common/repack_bits_byte.h"

namespace fengyun
{
    /*class SymbolReorg
    {
    private:
        volk::vector<uint8_t> d_buffer_reorg;

    public:
        int work(uint8_t *in, int size, uint8_t *out)
        {
            d_buffer_reorg.insert(d_buffer_reorg.end(), &in[0], &in[size]);
            int size_out = 0;
            int to_process = d_buffer_reorg.size() / 4;

            for (int i = 0; i < to_process; i++)
            {
                out[size_out++] = d_buffer_reorg[i * 4 + 0];
                out[size_out++] = d_buffer_reorg[i * 4 + 1];
                out[size_out++] = d_buffer_reorg[i * 4 + 3];
                out[size_out++] = d_buffer_reorg[i * 4 + 2];
            }

            d_buffer_reorg.erase(d_buffer_reorg.begin(), d_buffer_reorg.begin() + to_process * 4);

            return size_out;
        }

        void clear()
        {
            d_buffer_reorg.clear();
        }
    };*/

    class AHRPTViterbi2
    {
    private:
        // Settings
        const float d_ber_thresold;
        const int d_outsync_after;
        const int d_buffer_size;

        // Variables
        int d_outsinc;
        int d_state;
        bool d_first;
        float d_bers[2][2];
        float d_ber;

        // BER Decoders
        fec::depuncture_bb_impl depunc_ber;
        fec::code::cc_decoder_impl cc_decoder_in_ber;
        fec::code::cc_encoder_impl cc_encoder_in_ber;

        // BER Buffers
        uint8_t d_ber_test_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_input_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_input_reorg_buffer[TEST_BITS_LENGTH];
        uint8_t d_ber_input_buffer_depunc[TEST_BITS_LENGTH * 2]; // 1.5
        uint8_t d_ber_decoded_buffer[TEST_BITS_LENGTH * 2];
        uint8_t d_ber_encoded_buffer[TEST_BITS_LENGTH * 2]; // 1.5

        // Current phase status
        sathelper::PhaseShift d_phase_shift;
        bool d_iq_inv;
        int d_skip, d_skip_perm;
        sathelper::PacketFixer phaseShifter;

        // Work buffers
        uint8_t *fixed_soft_packet;
        uint8_t *converted_buffer;
        //uint8_t *reorg_buffer;
        uint8_t *depunc_buffer;
        uint8_t *output_buffer;

        // Main decoder
        fec::code::cc_decoder_impl cc_decoder_in;
        fec::depuncture_bb_impl depunc;
        //SymbolReorg reorg;

        // Repacker
        RepackBitsByte repacker;

        float getBER(uint8_t *input);

    public:
        AHRPTViterbi2(float ber_threshold, int outsync_after, int buffer_size);
        ~AHRPTViterbi2();

        int work(uint8_t *input, int size, uint8_t *output);
        float ber();
        int getState();
    };
} // namespace npp