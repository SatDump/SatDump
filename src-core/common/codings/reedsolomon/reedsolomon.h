#pragma once

#include <cstdint>
extern "C"
{
#include <correct.h>
}

// RS Decoding based on SatHelper's implementation
namespace reedsolomon
{
    enum RS_TYPE
    {
        RS223,
        RS239
    };

    class ReedSolomon
    {
    private:
        correct_reed_solomon *rs;
        int d_coded_bits;
        uint8_t buff[255];
        uint8_t odata[255];

    public:
        ReedSolomon(RS_TYPE type);
        ~ReedSolomon();

        void decode_interlaved(uint8_t *data, bool ccsds, int i, int *errors);
        int decode(uint8_t *data, bool ccsds);

        void encode_interlaved(uint8_t *data, bool ccsds, int i);
        void encode(uint8_t *data, bool ccsds);

        void deinterleave(uint8_t *data, uint8_t *output, uint8_t pos, uint8_t i);
        void interleave(uint8_t *data, uint8_t *output, uint8_t pos, uint8_t i);
    };
};