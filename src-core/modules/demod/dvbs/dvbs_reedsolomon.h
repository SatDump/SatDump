#pragma once

#include <cstdint>
extern "C"
{
#include "libs/correct/reed-solomon.h"
}

namespace dvbs
{
    class DVBSReedSolomon
    {
    private:
        correct_reed_solomon *rs;
        uint8_t buffer[255];
        uint8_t obuffer[255];

    public:
        DVBSReedSolomon()
        {
            rs = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_4_3_2_0, 0, 1, 16);
            memset(buffer, 0, 255);
            memset(obuffer, 0, 255);
        }

        ~DVBSReedSolomon()
        {
            correct_reed_solomon_destroy(rs);
        }

        int decode(uint8_t *data)
        {
            memset(&buffer[0], 0, 51);            // Leave the 51 first ones empty
            memcpy(&buffer[51], &data[0], 188);   // Copy next 188 bytes
            memcpy(&buffer[239], &data[188], 16); // Copy parity

            int err = correct_reed_solomon_decode(this->rs, buffer, 255, obuffer);

            if (err == 1)
                return -1;

            err = 0;

            // Calculate wrong bytes
            for (int i = 51; i < 239; i++)
            {
                if ((buffer[i] ^ obuffer[i]) != 0)
                    err++;
            }

            memcpy(data, &obuffer[51], 188); // Copy back

            return err;
        }
    };
};