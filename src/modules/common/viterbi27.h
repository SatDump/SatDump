/*
 * viterbi27.h
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */

#ifndef SATHELPER_INCLUDES_VITERBI27_H_
#define SATHELPER_INCLUDES_VITERBI27_H_

#include <cstdint>
#include <SatHelper/extensions.h>

namespace SatHelper {
#define _VITERBI27_POLYA 0x4F
#define _VITERBI27_POLYB 0x6D

    class Viterbi27 {
    private:
        int polynomials[2];
        uint8_t *checkDataPointer;
        void *viterbi;
        int frameBits;
        int BER;
        bool calculateErrors;

        static uint32_t calculateError(uint8_t *original, uint8_t *corrected, int length);

        void decode_generic(uint8_t *input, uint8_t *output);
        void encode_generic(uint8_t *input, uint8_t *output);
        void decode_sse4(uint8_t *input, uint8_t *output);
        void encode_sse4(uint8_t *input, uint8_t *output);

        void (Viterbi27::*_encode)(uint8_t *input, uint8_t *output) = NULL;
        void (Viterbi27::*_decode)(uint8_t *input, uint8_t *output) = NULL;
    public:
        Viterbi27(int frameBits, int polyA, int polyB);
        Viterbi27(int frameBits) :
            Viterbi27(frameBits, _VITERBI27_POLYA, _VITERBI27_POLYB) {
        }

        inline int DecodedSize() {
            return this->frameBits / 8;
        }
        inline int EncodedSize() {
            return this->frameBits * 2;
        }
        inline void SetCalculateErrors(bool calculateErrors) {
            this->calculateErrors = calculateErrors;
        }
        inline int GetBER() {
            return this->BER / 2;
        }
        inline float GetPercentBER() {
            return (100.f * this->BER) / this->frameBits;
        }

        inline bool IsSSEMode() {
            return SatHelper::Extensions::hasSSE4;
        }

        inline void decode(uint8_t *input, uint8_t *output) { (*this.*_decode)(input, output); }
        inline void encode(uint8_t *input, uint8_t *output) { (*this.*_encode)(input, output); }

        ~Viterbi27();
    };

}

#endif /* SATHELPER_INCLUDES_VITERBI27_H_ */
