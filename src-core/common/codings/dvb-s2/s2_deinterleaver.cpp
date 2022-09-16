#include "s2_deinterleaver.h"

namespace dvbs2
{
    S2Deinterleaver::S2Deinterleaver(dvbs2_constellation_t constellation, dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
    {
        if (framesize == FECFRAME_NORMAL)
            frame_length = 64800;
        else if (framesize == FECFRAME_SHORT)
            frame_length = 16200;

        if (constellation == MOD_QPSK)
        {
            rows = 0; // No interleaving
            mod_bits = 2;
        }
        else if (constellation == MOD_8PSK)
        {
            if (framesize == FECFRAME_NORMAL)
                rows = 21600;
            else if (framesize == FECFRAME_SHORT)
                rows = 5400;
            mod_bits = 3;

            if (rate == dvbs2::C3_5)
            {
                column0 = rows * 2;
                column1 = rows;
                column2 = 0;
            }
            else
            {
                column0 = 0;
                column1 = rows;
                column2 = rows * 2;
            }
        }
        else if (constellation == MOD_16APSK)
        {
            if (framesize == FECFRAME_NORMAL)
                rows = 16200;
            else if (framesize == FECFRAME_SHORT)
                rows = 4050;
            mod_bits = 4;

            column0 = 0;
            column1 = rows;
            column2 = rows * 2;
            column3 = rows * 3;
        }
        else if (constellation == MOD_32APSK)
        {
            if (framesize == FECFRAME_NORMAL)
                rows = 12960;
            else if (framesize == FECFRAME_SHORT)
                rows = 3240;
            mod_bits = 5;

            column0 = 0;
            column1 = rows;
            column2 = rows * 2;
            column3 = rows * 3;
            column4 = rows * 4;
        }
    }

    S2Deinterleaver::~S2Deinterleaver()
    {
    }

    void S2Deinterleaver::deinterleave(int8_t *input, int8_t *output)
    {
        /*
        In testing I found doing it this way with branching was faster than a more universal approach,
        hence the way it is done here.
        */
        if (mod_bits == 2)
        {
            for (int i = 0; i < frame_length / 2; i++)
            {
                output[i * 2 + 1] = input[i * 2 + 0];
                output[i * 2 + 0] = input[i * 2 + 1];
            }
        }
        else if (mod_bits == 3)
        {
            int8_t *c1 = &output[column0];
            int8_t *c2 = &output[column1];
            int8_t *c3 = &output[column2];

            int i = 0;
            for (int j = 0; j < rows; j++)
            {
                c1[j] = input[i++];
                c2[j] = input[i++];
                c3[j] = input[i++];
            }
        }
        else if (mod_bits == 4)
        {
            int8_t *c1 = &output[column0];
            int8_t *c2 = &output[column1];
            int8_t *c3 = &output[column2];
            int8_t *c4 = &output[column3];

            int i = 0;
            for (int j = 0; j < rows; j++)
            {
                c1[j] = input[i++];
                c2[j] = input[i++];
                c3[j] = input[i++];
                c4[j] = input[i++];
            }
        }
        else if (mod_bits == 5)
        {
            column4 = rows * 4;

            int8_t *c1 = &output[column0];
            int8_t *c2 = &output[column1];
            int8_t *c3 = &output[column2];
            int8_t *c4 = &output[column3];
            int8_t *c5 = &output[column4];

            int i = 0;
            for (int j = 0; j < rows; j++)
            {
                c1[j] = input[i++];
                c2[j] = input[i++];
                c3[j] = input[i++];
                c4[j] = input[i++];
                c5[j] = input[i++];
            }
        }
    }

    void S2Deinterleaver::interleave(uint8_t *input, uint8_t *output)
    {
        /*
        In testing I found doing it this way with branching was faster than a more universal approach,
        hence the way it is done here.
        */
        if (mod_bits == 2)
        {
            for (int i = 0; i < frame_length / 2; i++)
            {
                output[i * 2 + 0] = input[i * 2 + 1];
                output[i * 2 + 1] = input[i * 2 + 0];
            }
        }
        else if (mod_bits == 3)
        {
            uint8_t *c1 = &input[column0];
            uint8_t *c2 = &input[column1];
            uint8_t *c3 = &input[column2];

            int i = 0;
            for (int j = 0; j < rows; j++)
            {
                output[i++] = c1[j];
                output[i++] = c2[j];
                output[i++] = c3[j];
            }
        }
        else if (mod_bits == 4)
        {
            uint8_t *c1 = &input[column0];
            uint8_t *c2 = &input[column1];
            uint8_t *c3 = &input[column2];
            uint8_t *c4 = &input[column3];

            int i = 0;
            for (int j = 0; j < rows; j++)
            {
                output[i++] = c1[j];
                output[i++] = c2[j];
                output[i++] = c3[j];
                output[i++] = c4[j];
            }
        }
        else if (mod_bits == 5)
        {
            column4 = rows * 4;

            uint8_t *c1 = &input[column0];
            uint8_t *c2 = &input[column1];
            uint8_t *c3 = &input[column2];
            uint8_t *c4 = &input[column3];
            uint8_t *c5 = &input[column4];

            int i = 0;
            for (int j = 0; j < rows; j++)
            {
                output[i++] = c1[j];
                output[i++] = c2[j];
                output[i++] = c3[j];
                output[i++] = c4[j];
                output[i++] = c5[j];
            }
        }
    }
}