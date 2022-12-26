#pragma once

#include "matrix/sparse_matrix.h"
#include <map>
#include <functional>

// Based on gr-ccsds
namespace codings
{
    namespace ldpc
    {
        class LDPCDecoder
        {
        public:
            LDPCDecoder(Sparse_matrix pcm);
            virtual ~LDPCDecoder();
            virtual int decode(uint8_t *out, const int8_t *in, int it) = 0;
            virtual int simd() = 0;
        };

        struct GetLDPCDecodersEvent
        {
            std::map<std::string, std::function<LDPCDecoder *(Sparse_matrix)>> &decoder_list;
        };

        LDPCDecoder *get_best_ldpc_decoder(Sparse_matrix pcm);
    }
}
