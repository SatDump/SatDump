/*
 * correlator.h
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */

#ifndef INCLUDES_CORRELATOR_H_
#define INCLUDES_CORRELATOR_H_

#include <vector>
#include <cstdint>
#include <stdint.h>
#include <cstdio>

namespace SatHelper {

    typedef std::vector<uint8_t> VecU8;

    class Correlator {
    private:
        std::vector<VecU8> words;
        std::vector<uint32_t> correlation;
        std::vector<uint32_t> tmpCorrelation;
        std::vector<uint32_t> position;
        uint32_t wordNumber;
        uint8_t currentWordSize;

        // Cache Start of the array, so the loop doesn't call vector[]
        uint8_t **wordsPtr;
        uint32_t *tmpCorrelationPtr;
        uint32_t *correlationPtr;
        uint32_t *positionPtr;

        void resetCorrelation();
        void updatePointers();
    public:

        static inline bool hardCorrelate(uint8_t dataByte, uint8_t wordByte) {
            return ((dataByte >= 127) && (wordByte == 0)) || ((dataByte < 127) && (wordByte == 255));
        }

        static inline uint32_t softCorrelate(uint8_t dataByte, uint8_t wordByte) {
            // TODO: This doesn't work
            /*
            int a = dataByte - 128; // 127
            int b = wordByte - 128; // 127  |
            return (uint32_t) ((a + b) * (a + b)); // Its a modulus, should NEVER be negative.
            */
            return hardCorrelate(dataByte, wordByte);
        }

        Correlator();
        ~Correlator();

        inline uint32_t getHighestCorrelation() {
            return correlation[wordNumber];
        }

        inline uint32_t getHighestCorrelationPosition() {
            return position[wordNumber];
        }

        inline uint32_t getCorrelationWordNumber() {
            return wordNumber;
        }

        void addWord(uint32_t word);
        void addWord(uint64_t word);

        void correlate(uint8_t *data, uint32_t length);
    };

}

#endif /* INCLUDES_CORRELATOR_H_ */
