#include "decode_utils.h"
#include <cstring>

#include <iostream>

namespace inmarsat
{
    namespace aero
    {
        void deinterleave(int8_t *input, int8_t *output, int cols)
        {
            const int rows = 64;
            int k = 0;
            for (int j = 0; j < cols; j++)
                for (int i = 0; i < rows; i++)
                    output[k++] = input[((i * 27) % rows) * cols + j];
        }
    }
}