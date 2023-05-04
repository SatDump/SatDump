#pragma once

#include <cstdint>
#include <cstring>

namespace viterbi
{
    namespace puncturing
    {
        class GenericDepunc
        {
        public:
            virtual int depunc_static(uint8_t *in, uint8_t *out, int size, int shift) = 0; // To find BER
            virtual void set_shift(int shift) = 0;                                         // Set shift of the continuous depunc
            virtual int depunc_cont(uint8_t *in, uint8_t *out, int size) = 0;              // Continous depuncturing
            virtual int get_numstates() = 0;                                               // Numstates of depunc
            virtual float get_berscale() = 0;                                              // BER Scale
        };

        class Depunc23 : public GenericDepunc
        {
        private:
            bool is_first = false;
            int changing_shift = 0;
            int got_extra = false;
            uint8_t buf = 128;

        public:
            int depunc_static(uint8_t *in, uint8_t *out, int size, int shift)
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

            void set_shift(int shift)
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

            int get_numstates() { return 3; }
            float get_berscale() { return 3.5; }
        };

        class Depunc34 : public GenericDepunc
        {
        private:
            bool is_first = false;
            int changing_shift = 0;
            int got_extra = false;
            uint8_t buf = 128;

        public:
            int depunc_static(uint8_t *in, uint8_t *out, int size, int shift)
            {
                int oo = 0;
                int actual_shift = shift % 4;

                if (shift > 3)
                    out[oo++] = 128;

                for (int i = 0; i < size; i++)
                {
                    if ((i + actual_shift) % 4 == 0)
                        out[oo++] = in[i];
                    else if ((i + actual_shift) % 4 == 1)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if ((i + actual_shift) % 4 == 2)
                        out[oo++] = in[i];
                    else if ((i + actual_shift) % 4 == 3)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                }

                return oo;
            }

            void set_shift(int shift)
            {
                changing_shift = shift;
                is_first = shift > 3;
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

                changing_shift = changing_shift % 4;

                for (int i = 0; i < size; i++)
                {
                    if (changing_shift % 4 == 0)
                        out[oo++] = in[i];
                    else if (changing_shift % 4 == 1)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if (changing_shift % 4 == 2)
                        out[oo++] = in[i];
                    else if (changing_shift % 4 == 3)
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

            int get_numstates() { return 4; }
            float get_berscale() { return 5; }
        };

        class Depunc56 : public GenericDepunc
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

            int get_numstates() { return 6; }
            float get_berscale() { return 8; }
        };

        class Depunc78 : public GenericDepunc
        {
        private:
            bool is_first = false;
            int changing_shift = 0;
            int got_extra = false;
            uint8_t buf = 128;

        public:
            int depunc_static(uint8_t *in, uint8_t *out, int size, int shift)
            {
                int oo = 0;
                int actual_shift = shift % 8;

                if (shift > 7)
                    out[oo++] = 128;

                for (int i = 0; i < size; i++)
                {
                    if ((i + actual_shift) % 8 == 0)
                        out[oo++] = in[i];
                    else if ((i + actual_shift) % 8 == 1)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if ((i + actual_shift) % 8 == 2)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if ((i + actual_shift) % 8 == 3)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if ((i + actual_shift) % 8 == 4)
                        out[oo++] = in[i];
                    else if ((i + actual_shift) % 8 == 5)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if ((i + actual_shift) % 8 == 6)
                    {
                        out[oo++] = 128;
                        out[oo++] = in[i];
                    }
                    else if ((i + actual_shift) % 8 == 7)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                }

                return oo;
            }

            void set_shift(int shift)
            {
                changing_shift = shift;
                is_first = shift > 7;
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

                changing_shift = changing_shift % 8;

                for (int i = 0; i < size; i++)
                {
                    if (changing_shift % 8 == 0)
                        out[oo++] = in[i];
                    else if (changing_shift % 8 == 1)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if (changing_shift % 8 == 2)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if (changing_shift % 8 == 3)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if (changing_shift % 8 == 4)
                        out[oo++] = in[i];
                    else if (changing_shift % 8 == 5)
                    {
                        out[oo++] = in[i];
                        out[oo++] = 128;
                    }
                    else if (changing_shift % 8 == 6)
                    {
                        out[oo++] = 128;
                        out[oo++] = in[i];
                    }
                    else if (changing_shift % 8 == 7)
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

            int get_numstates() { return 8; }
            float get_berscale() { return 10; }
        };
    }
}