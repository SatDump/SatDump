/*
 * correlator.cpp
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */

#include "correlator.h"
#include "exceptions/WordSizeException.h"
#include <memory.h>

#define USE_UNROLLING

namespace sathelper
{

    Correlator::Correlator()
    {
        wordNumber = 0;
        currentWordSize = 0;
        wordsPtr = NULL;
        updatePointers();
    }

    Correlator::~Correlator()
    {
        if (wordsPtr != NULL)
        {
            delete[] wordsPtr;
        }
    }

    void Correlator::addWord(uint32_t word)
    {
        if (currentWordSize != 0 && currentWordSize != 32)
        {
            throw WordSizeException(32, currentWordSize);
        }
        VecU8 wordVec;

        for (int i = 0; i < 32; i++)
        {
            wordVec.push_back((word >> (32 - i - 1)) & 1 ? 0xFF : 0x00);
        }

        words.push_back(wordVec);
        correlation.push_back(0);
        position.push_back(0);
        tmpCorrelation.push_back(0);
        updatePointers();
    }

    void Correlator::addWord(uint64_t word)
    {
        if (currentWordSize != 0 && currentWordSize != 64)
        {
            throw WordSizeException(64, currentWordSize);
        }
        VecU8 wordVec;

        for (int i = 0; i < 64; i++)
        {
            wordVec.push_back((word >> (64 - i - 1) & 1) ? 0xFF : 0x00);
        }

        words.push_back(wordVec);
        correlation.push_back(0);
        position.push_back(0);
        tmpCorrelation.push_back(0);
        updatePointers();
    }

    void Correlator::resetCorrelation()
    {
        int numWords = words.size();
        if (words.size() > 0)
        {
            memset(correlationPtr, 0x00, numWords * sizeof(uint32_t));
            memset(positionPtr, 0x00, numWords * sizeof(uint32_t));
            memset(tmpCorrelationPtr, 0x00, numWords * sizeof(uint32_t));
        }
    }

    void Correlator::updatePointers()
    {
        // Cache Start of the array, so the loop doesn't call vector[]
        if (wordsPtr != NULL)
        {
            delete[] wordsPtr;
        }

        if (words.size() > 0)
        {
            wordsPtr = new uint8_t *[words.size()];
            for (unsigned int i = 0; i < words.size(); i++)
            {
                wordsPtr[i] = &words[i][0];
            }

            if (tmpCorrelation.size() > 0)
            {
                tmpCorrelationPtr = &tmpCorrelation[0];
            }

            if (correlation.size() > 0)
            {
                correlationPtr = &correlation[0];
            }
            if (position.size() > 0)
            {
                positionPtr = &position[0];
            }
        }
    }

#ifndef USE_UNROLLING

    void Correlator::correlate(uint8_t *data, uint32_t length)
    {
        int wordSize = words[0].size();
        int numWords = words.size();
        int maxSearch = length - wordSize;
        resetCorrelation();

        for (int i = 0; i < maxSearch; i++)
        {
            for (int n = 0; n < numWords; n++)
            {
                tmpCorrelationPtr[n] = 0;
            }

            for (int k = 0; k < wordSize; k++)
            {
                for (int n = 0; n < numWords; n++)
                {
                    tmpCorrelationPtr[n] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[n][k]);
                }
            }

            for (int n = 0; n < numWords; n++)
            {
                if (tmpCorrelationPtr[n] > correlationPtr[n])
                {
                    correlationPtr[n] = tmpCorrelationPtr[n];
                    positionPtr[n] = i;
                    tmpCorrelationPtr[n] = 0;
                }
            }
        }

        uint32_t corr = 0;
        for (int n = 0; n < numWords; n++)
        {
            if (correlationPtr[n] > corr)
            {
                wordNumber = n;
                corr = correlationPtr[n];
            }
        }
    }
#else

    void Correlator::correlate(uint8_t *data, uint32_t length)
    {
        int wordSize = words[0].size();
        int numWords = words.size();
        int maxSearch = length - wordSize;
        resetCorrelation();

        if (numWords == 2)
        { // Optimization for BPSK Phase
            for (int i = 0; i < maxSearch; i++)
            {
                tmpCorrelationPtr[0] = 0;
                tmpCorrelationPtr[1] = 0;

                for (int k = 0; k < wordSize; k++)
                {
                    tmpCorrelationPtr[0] += softCorrelate(data[i + k], wordsPtr[0][k]); //(uint32_t) Correlator::hardCorrelate(data[i + k], words[0][k]);
                    tmpCorrelationPtr[1] += softCorrelate(data[i + k], wordsPtr[1][k]); //(uint32_t) Correlator::hardCorrelate(data[i + k], words[1][k]);
                }

                if (tmpCorrelationPtr[0] > correlationPtr[0])
                {
                    correlationPtr[0] = tmpCorrelationPtr[0];
                    positionPtr[0] = i;
                    tmpCorrelationPtr[0] = 0;
                }

                if (tmpCorrelationPtr[1] > correlationPtr[1])
                {
                    correlationPtr[1] = tmpCorrelationPtr[1];
                    positionPtr[1] = i;
                    tmpCorrelationPtr[1] = 0;
                }
            }
        }
        else if (numWords == 4)
        { // Optimization for QPSK Phase
            for (int i = 0; i < maxSearch; i++)
            {
                tmpCorrelationPtr[0] = 0;
                tmpCorrelationPtr[1] = 0;
                tmpCorrelationPtr[2] = 0;
                tmpCorrelationPtr[3] = 0;

                for (int k = 0; k < wordSize; k++)
                {
                    tmpCorrelationPtr[0] += softCorrelate(data[i + k], wordsPtr[0][k]); //(uint32_t) Correlator::hardCorrelate(data[i + k], words[0][k]);
                    tmpCorrelationPtr[1] += softCorrelate(data[i + k], wordsPtr[1][k]); //(uint32_t) Correlator::hardCorrelate(data[i + k], words[1][k]);
                    tmpCorrelationPtr[2] += softCorrelate(data[i + k], wordsPtr[2][k]); //(uint32_t) Correlator::hardCorrelate(data[i + k], words[2][k]);
                    tmpCorrelationPtr[3] += softCorrelate(data[i + k], wordsPtr[3][k]); //(uint32_t) Correlator::hardCorrelate(data[i + k], words[3][k]);
                }

                if (tmpCorrelationPtr[0] > correlationPtr[0])
                {
                    correlationPtr[0] = tmpCorrelationPtr[0];
                    positionPtr[0] = i;
                    tmpCorrelationPtr[0] = 0;
                }

                if (tmpCorrelationPtr[1] > correlationPtr[1])
                {
                    correlationPtr[1] = tmpCorrelationPtr[1];
                    positionPtr[1] = i;
                    tmpCorrelationPtr[1] = 0;
                }

                if (tmpCorrelationPtr[2] > correlationPtr[2])
                {
                    correlationPtr[2] = tmpCorrelationPtr[2];
                    positionPtr[2] = i;
                    tmpCorrelationPtr[2] = 0;
                }

                if (tmpCorrelationPtr[3] > correlationPtr[3])
                {
                    correlationPtr[3] = tmpCorrelationPtr[3];
                    positionPtr[3] = i;
                    tmpCorrelationPtr[3] = 0;
                }
            }
        }
        else
        { // Other use cases
            // Duff's Device Loop Unfolding
            int c;
            int n;
            for (int i = 0; i < maxSearch; i++)
            {
                memset(tmpCorrelationPtr, 0x00, numWords * sizeof(uint32_t));
                for (int k = 0; k < wordSize; k++)
                {
                    c = 0;
                    n = (numWords + 7) / 8;
                    switch (numWords % 8)
                    {
                    case 0:
                        do
                        {
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 7:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 6:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 5:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 4:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 3:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 2:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                            [[fallthrough]];
                        case 1:
                            tmpCorrelationPtr[c] += (uint32_t)Correlator::hardCorrelate(data[i + k], wordsPtr[c][k]);
                            c++;
                        } while (--n > 0);
                    }
                }

                c = 0;
                n = (numWords + 7) / 8;
                switch (numWords % 8)
                {
                case 0:
                    do
                    {
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 7:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 6:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 5:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 4:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 3:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 2:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                        [[fallthrough]];
                    case 1:
                        if (tmpCorrelationPtr[c] > correlationPtr[c])
                        {
                            correlationPtr[c] = tmpCorrelationPtr[c];
                            positionPtr[c] = i;
                            tmpCorrelationPtr[c] = 0;
                        }
                        c++;
                    } while (--n > 0);
                }
            }
        }

        uint32_t corr = 0;
        for (int n = 0; n < numWords; n++)
        {
            if (correlationPtr[n] > corr)
            {
                wordNumber = n;
                corr = correlationPtr[n];
            }
        }
    }
#endif
} // namespace sathelper