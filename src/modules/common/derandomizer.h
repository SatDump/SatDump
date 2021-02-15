/*
 * DeRandomizer.h
 *
 *  Created on: 07/11/2016
 *      Author: Lucas Teske
 */

#ifndef INCLUDES_DERANDOMIZER_H_
#define INCLUDES_DERANDOMIZER_H_

#include <cstdint>

namespace SatHelper {
    class DeRandomizer {
    private:
        static uint8_t pn[255];
    public:
        static void DeRandomize(uint8_t *data, int length);
    };
}

#endif /* INCLUDES_DERANDOMIZER_H_ */
