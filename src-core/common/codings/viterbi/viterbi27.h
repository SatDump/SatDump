#pragma once

#include "cc_decoder.h"
#include "cc_encoder.h"

namespace viterbi
{
    static std::vector<int> CCSDS_R2_K7_POLYS = {79, 109};

    class Viterbi27
    {
    private:
        CCDecoder cc_decoder_in;
        CCEncoder cc_encoder_in;
        float d_ber;
        const int d_ber_test_size;
        const int d_frame_size;

        uint8_t *hard_buffer;
        uint8_t *buffer_deco;
        uint8_t *buffer_enco;

        int bitc, bytec;
        uint8_t shifter;

    public:
        Viterbi27(int frame_size, std::vector<int> polys, int ber_test_size = 1024);
        ~Viterbi27();

        void work(int8_t *in, uint8_t *out, bool isHard = false);

        float ber();
    };
}
