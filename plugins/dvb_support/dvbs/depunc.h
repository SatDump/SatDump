#pragma once

#include <cstdint>
#include <cstring>

namespace viterbi
{
    class Depunc23
    {
    private:
        bool is_first = false;
        int changing_shift = 0;
        int got_extra = false;
        uint8_t buf = 128;

    public:
        int depunc_static(uint8_t *in, uint8_t *out, int size, int shift) // To find BER
        {
            int oo = 0;
            int actual_shift = shift % 3;

            if (shift > 2)
                out[oo++] = 128;

            for (int i = 0; i < size; i++)
            {
                if ((i + actual_shift) % 3 == 0)
                    out[oo++] = in[i];
                else if ((i + actual_shift) % 3 == 1)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                else if ((i + actual_shift) % 3 == 2)
                    out[oo++] = in[i];
            }

            return oo;
        }

        void set_shift(int shift) // Set shift of the continuous depunc
        {
            changing_shift = shift;
            is_first = shift > 2;
        }

        int depunc_cont(uint8_t *in, uint8_t *out, int size)
        {
            int oo = 0;

            if (is_first || got_extra)
            {
                out[oo++] = buf;
                is_first = false;
                got_extra = false;
            }

            changing_shift = changing_shift % 3;

            for (int i = 0; i < size; i++)
            {
                if (changing_shift % 3 == 0)
                    out[oo++] = in[i];
                else if (changing_shift % 3 == 1)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                else if (changing_shift % 3 == 2)
                    out[oo++] = in[i];
                changing_shift++;
            }

            if (oo % 2 == 1)
            {
                buf = out[oo - 1];
                oo -= 1;
                got_extra = true;
            }

            return oo;
        }
    };

    class Depunc56
    {
    private:
        bool is_first = false;
        int changing_shift = 0;
        int got_extra = false;
        uint8_t buf = 128;

    public:
        int depunc_static(uint8_t *in, uint8_t *out, int size, int shift) // To find BER
        {
            int oo = 0;
            int actual_shift = shift % 6;

            if (shift > 5)
                out[oo++] = 128;

            for (int i = 0; i < size; i++)
            {
                if ((i + actual_shift) % 6 == 0)
                    out[oo++] = in[i];
                else if ((i + actual_shift) % 6 == 1)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                else if ((i + actual_shift) % 6 == 2)
                    out[oo++] = in[i];
                else if ((i + actual_shift) % 6 == 3)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                else if ((i + actual_shift) % 6 == 4)
                {
                    out[oo++] = 128;
                    out[oo++] = in[i];
                }
                else if ((i + actual_shift) % 6 == 5)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
            }

            return oo;
        }

        void set_shift(int shift) // Set shift of the continuous depunc
        {
            changing_shift = shift;
            is_first = shift > 5;
        }

        int depunc_cont(uint8_t *in, uint8_t *out, int size)
        {
            int oo = 0;

            if (is_first || got_extra)
            {
                out[oo++] = buf;
                is_first = false;
                got_extra = false;
            }

            changing_shift = changing_shift % 6;

            for (int i = 0; i < size; i++)
            {
                if (changing_shift % 6 == 0)
                    out[oo++] = in[i];
                else if (changing_shift % 6 == 1)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                else if (changing_shift % 6 == 2)
                    out[oo++] = in[i];
                else if (changing_shift % 6 == 3)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                else if (changing_shift % 6 == 4)
                {
                    out[oo++] = 128;
                    out[oo++] = in[i];
                }
                else if (changing_shift % 6 == 5)
                {
                    out[oo++] = in[i];
                    out[oo++] = 128;
                }
                changing_shift++;
            }

            if (oo % 2 == 1)
            {
                buf = out[oo - 1];
                oo -= 1;
                got_extra = true;
            }

            return oo;
        }
    };
}