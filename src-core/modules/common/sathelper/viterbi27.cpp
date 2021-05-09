/*
 * viterbi27.cpp
 *
 *  Created on: 04/11/2016
 *      Author: lucas
 */

#include "SIMD/MemoryOp.h"

extern "C"
{
#include <correct.h>
#ifndef __ANDROID__
#ifndef __MINGW32__
#ifndef _MSC_VER
#ifdef MEMORY_OP_X86
#include <correct-sse.h>
#endif
#endif
#endif
#endif
}

#include "viterbi27.h"
#include <memory.h>
#include <cstring>
#include "correlator.h"
#include "exceptions/ViterbiCreationException.h"

namespace sathelper
{

    Viterbi27::Viterbi27(int frameBits, int polyA, int polyB)
    {
        this->frameBits = frameBits;
        this->polynomials[0] = polyA;
        this->polynomials[1] = polyB;
        this->BER = 0;
        this->calculateErrors = true;

#ifndef __ANDROID__
#ifndef __MINGW32__
#ifndef _MSC_VER
#ifdef MEMORY_OP_X86
        if (Extensions::hasSSE4)
        {
            viterbi = correct_convolutional_sse_create(2, 7, new uint16_t[2]{(uint16_t)polyA, (uint16_t)polyB});
            this->_encode = &Viterbi27::encode_sse4;
            this->_decode = &Viterbi27::decode_sse4;
        }
        else
#endif
#endif
#endif
#endif
        {
            viterbi = correct_convolutional_create(2, 7, new uint16_t[2]{(uint16_t)polyA, (uint16_t)polyB});
            this->_encode = &Viterbi27::encode_generic;
            this->_decode = &Viterbi27::decode_generic;
        }

        if (viterbi == NULL)
        {
            throw ViterbiCreationException();
        }
        this->checkDataPointer = new uint8_t[this->frameBits * 2];
    }

    Viterbi27::~Viterbi27()
    {
        delete[] this->checkDataPointer;
    }

    void Viterbi27::encode_sse4(uint8_t *input, uint8_t *output)
    {
#ifndef __ANDROID__
#ifndef __MINGW32__
#ifndef _MSC_VER
#ifdef MEMORY_OP_X86
        const int l = correct_convolutional_sse_encode_len((correct_convolutional_sse *)viterbi, this->DecodedSize());
        const int bl = l % 8 == 0 ? l / 8 : (l / 8) + 1;
        uint8_t *data = new uint8_t[bl];
        correct_convolutional_sse_encode((correct_convolutional_sse *)viterbi, input, this->DecodedSize(), data);

        // Convert to soft bits
        for (int i = 0; i < bl && i * 8 < this->EncodedSize(); i++)
        {
            uint8_t d = data[i];
            for (int z = 7; z >= 0; z--)
            {
                output[i * 8 + (7 - z)] = 0 - ((d & (1 << z)) == 0);
            }
        }
        delete[] data;
#endif
#endif
#endif
#endif
    }

    void Viterbi27::decode_sse4(uint8_t *input, uint8_t *output)
    {
#ifndef __ANDROID__
#ifndef __MINGW32__
#ifndef _MSC_VER
#ifdef MEMORY_OP_X86
        correct_convolutional_sse_decode_soft((correct_convolutional_sse *)viterbi, input, this->frameBits * 2, output);
        if (calculateErrors)
        {
            this->encode_sse4(output, this->checkDataPointer);
            this->BER = Viterbi27::calculateError(input, this->checkDataPointer, this->frameBits * 2);
        }
#endif
#endif
#endif
#endif
    }

    void Viterbi27::encode_generic(uint8_t *input, uint8_t *output)
    {
        const int l = correct_convolutional_encode_len((correct_convolutional *)viterbi, this->DecodedSize());
        const int bl = l % 8 == 0 ? l / 8 : (l / 8) + 1;
        uint8_t *data = new uint8_t[bl];
        correct_convolutional_encode((correct_convolutional *)viterbi, input, this->DecodedSize(), data);

        // Convert to soft bits
        for (int i = 0; i < bl && i * 8 < this->EncodedSize(); i++)
        {
            uint8_t d = data[i];
            for (int z = 7; z >= 0; z--)
            {
                output[i * 8 + (7 - z)] = 0 - ((d & (1 << z)) == 0);
            }
        }
        delete[] data;
    }

    void Viterbi27::decode_generic(uint8_t *input, uint8_t *output)
    {
        correct_convolutional_decode_soft((correct_convolutional *)viterbi, input, this->frameBits * 2, output);
        if (calculateErrors)
        {
            this->encode_generic(output, this->checkDataPointer);
            this->BER = Viterbi27::calculateError(input, this->checkDataPointer, this->frameBits * 2);
        }
    }

    uint32_t Viterbi27::calculateError(uint8_t *original, uint8_t *corrected, int length)
    {
        uint32_t errors = 0;
        for (int i = 0; i < length; i++)
        {
            errors += (uint32_t)Correlator::hardCorrelate(original[i], ~corrected[i]);
        }

        return errors;
    }
} // namespace sathelper