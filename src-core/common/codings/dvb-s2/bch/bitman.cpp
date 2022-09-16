#include "bitman.hh"

namespace dvbs2
{
    namespace CODE
    {
        void xor_be_bit(uint8_t *buf, int pos, bool val)
        {
            buf[pos / 8] ^= val << (7 - pos % 8);
        }

        void xor_le_bit(uint8_t *buf, int pos, bool val)
        {
            buf[pos / 8] ^= val << (pos % 8);
        }

        void set_be_bit(uint8_t *buf, int pos, bool val)
        {
            buf[pos / 8] = (~(1 << (7 - pos % 8)) & buf[pos / 8]) | (val << (7 - pos % 8));
        }

        void set_le_bit(uint8_t *buf, int pos, bool val)
        {
            buf[pos / 8] = (~(1 << (pos % 8)) & buf[pos / 8]) | (val << (pos % 8));
        }

        bool get_be_bit(uint8_t *buf, int pos)
        {
            return (buf[pos / 8] >> (7 - pos % 8)) & 1;
        }

        bool get_le_bit(uint8_t *buf, int pos)
        {
            return (buf[pos / 8] >> (pos % 8)) & 1;
        }
    } // namespace CODE
}