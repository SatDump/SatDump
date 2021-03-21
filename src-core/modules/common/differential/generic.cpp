#include "generic.h"

namespace diff
{
    GenericDiff::GenericDiff(unsigned int modulus) : d_modulus(modulus)
    {
    }

    int GenericDiff::work(uint8_t *in, int length, uint8_t *out)
    {
        tmp_.insert(tmp_.end(), in, &in[length]);

        uint8_t *input = &tmp_[1]; // ensure that input[-1] is valid

        unsigned modulus = d_modulus;

        for (int i = 0; i < (int)tmp_.size() - 2; i++)
        {
            out[i] = (input[i] - input[i - 1]) % modulus;
        }

        int out_size = tmp_.size() - 2;

        tmp_.erase(tmp_.begin(), tmp_.end() - 2); // History

        return out_size;
    }
} // namespace diff