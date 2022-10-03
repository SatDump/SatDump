#pragma once

#include <cstdint>

// CRC Implementation from
// GNU Radio. It's VERY generic
// so was pretty much perfect
// for the purpose...
namespace codings
{
    namespace crc
    {
        class GenericCRC
        {
        private:
            uint64_t d_table[256];
            unsigned d_num_bits;
            uint64_t d_mask;
            uint64_t d_initial_value;
            uint64_t d_final_xor;
            bool d_input_reflected;
            bool d_result_reflected;

        public:
            GenericCRC(unsigned num_bits,
                       uint64_t poly,
                       uint64_t initial_value,
                       uint64_t final_xor,
                       bool input_reflected,
                       bool result_reflected);

            uint64_t compute(uint8_t *data, unsigned int len);
            uint64_t reflect(uint64_t word) const;
        };
    }
}