/*
 * reedsolomon.h
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */

#ifndef INCLUDES_REEDSOLOMON_H_
#define INCLUDES_REEDSOLOMON_H_

#include <cstdint>

extern "C"
{
#include <correct.h>
}

namespace SatHelper
{
    class ReedSolomon
    {
    private:
        bool copyParityToOutput;
        correct_reed_solomon *rs;

    public:
        ReedSolomon();

        // CCSDS standard (255,223) RS codec with dual-basis symbol representation
        uint32_t decode_ccsds(uint8_t *data);

        // CCSDS standard (255,223) RS codec with conventional (*not* dual-basis) symbol representation
        uint32_t decode_rs8(uint8_t *data);

        // Deinterleave by I
        void deinterleave(uint8_t *data, uint8_t *output, uint8_t pos, uint8_t I);

        // Interleave by I
        void interleave(uint8_t *data, uint8_t *output, uint8_t pos, uint8_t I);

        inline void SetCopyParityToOutput(bool copyParityToOutput)
        {
            this->copyParityToOutput = copyParityToOutput;
        }

        ~ReedSolomon();
    };
} // namespace SatHelper

#endif /* INCLUDES_REEDSOLOMON_H_ */
