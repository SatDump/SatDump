/*
 * DifferentialDecoder.h
 *
 *  Created on: 25/01/2017
 *      Author: Lucas Teske
 */

#ifndef SRC_DIFFERENTIALDECODER_H_
#define SRC_DIFFERENTIALDECODER_H_

#include <cstdint>

namespace SatHelper
{

    class DifferentialEncoding
    {
    public:
        static void nrzmDecode(uint8_t *data, int length);
    };

} /* namespace SatHelper */

#endif /* SRC_DIFFERENTIALDECODER_H_ */
