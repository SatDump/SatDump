#include "deint.h"
#include <cstring>
#include <vector>

#define LEN(x) (sizeof(x) / sizeof(*x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

namespace meteor
{
    const uint8_t _syncwords[] = {0x27, 0x4E, 0xD8, 0xB1};

    inline int count_ones(uint64_t v)
    {
        int count;
        for (count = 0; v; count++)
            v &= v - 1;
        return count;
    }

    int DeinterleaverReader::autocorrelate(phase_t *rotation, int period, uint8_t *hard, int len)
    {
        int i, j, k;
        uint8_t tmp, _xor, window;
        std::vector<int> ones_count(8 * period, 0);
        std::vector<int> average_bit(8 * period + 8, 0);
        int corr, best_corr, best_idx;

        /* Make len a multiple of the period */
        len -= len % period;

        /* XOR the bitstream with a delayed version of itself */
        for (i = 0; i < period; i++)
        {
            j = len - period + i - 1;
            tmp = hard[j];
            for (j -= period; j >= 0; j -= period)
            {
                _xor = hard[j] ^ tmp;
                tmp = hard[j];
                hard[j] = _xor;

                /* Keep track of the average value of each bit in the period window */
                for (k = 0; k < 8; k++)
                {
                    average_bit[8 * i + 7 - k] += tmp & (1 << k) ? 1 : -1;
                }
            }
        }

        /* Find the bit offset with the most zeroes */
        window = 0;
        hard--;
        for (i = 0; i < 8 * (len - period); i++)
        {
            if (!(i % 8))
                hard++;
            window = (window >> 1) | ((*hard << (i % 8)) & 0x80);
            ones_count[i % (8 * period)] += count_ones(window);
        }

        best_idx = 0;
        best_corr = ones_count[0] - len / 64; /* Give offset 0 a small boost */
        for (i = 1; i < (int)ones_count.size(); i++)
        {
            if (ones_count[i] < best_corr)
            {
                best_corr = ones_count[i];
                best_idx = i;
            }
        }

        /* Collect the average syncword bits */
        tmp = 0;
        for (i = 7; i >= 0; i--)
        {
            tmp |= (average_bit[best_idx + i] > 0 ? 1 << i : 0);
        }

        /* Find the phase rotation of the syncword */
        *rotation = (phase_t)0;
        best_corr = count_ones(tmp ^ _syncwords[0]);
        for (i = 1; i < (int)LEN(_syncwords); i++)
        {
            corr = count_ones(tmp ^ _syncwords[i]);
            if (best_corr > corr)
            {
                best_corr = corr;
                *rotation = (phase_t)i;
            }
        }

        return best_idx;
    }

    void DeinterleaverReader::deinterleave(int8_t *dst, const int8_t *src, size_t len)
    {
        int delay, write_idx, read_idx;
        size_t i;

        read_idx = (_offset + INTER_BRANCH_COUNT * INTER_BRANCH_DELAY) % sizeof(_deint);
        // assert(len < sizeof(_deint));

        /* Write bits to the deinterleaver */
        for (i = 0; i < len; i++)
        {
            /* Skip sync marker */
            if (!_cur_branch)
                src += 8;

            /* Compute the delay of the current symbol based on the branch we're on */
            delay = (_cur_branch % INTER_BRANCH_COUNT) * INTER_BRANCH_DELAY * INTER_BRANCH_COUNT;
            write_idx = (_offset - delay + sizeof(_deint)) % sizeof(_deint);

            _deint[write_idx] = *src++;

            _offset = (_offset + 1) % sizeof(_deint);
            _cur_branch = (_cur_branch + 1) % INTER_MARKER_INTERSAMPS;
        }

        /* Read bits from the deinterleaver */
        for (; len > 0; len--)
        {
            *dst++ = _deint[read_idx];
            read_idx = (read_idx + 1) % sizeof(_deint);
        }
    }

    size_t DeinterleaverReader::deinterleave_num_samples(size_t output_count)
    {
        int num_syncs;

        if (!output_count)
            return 0;

        num_syncs = (_cur_branch ? 0 : 1) + (output_count - (INTER_MARKER_INTERSAMPS - _cur_branch) + INTER_MARKER_INTERSAMPS - 1) / INTER_MARKER_INTERSAMPS;

        return output_count + 8 * num_syncs;
    }

    int DeinterleaverReader::deinterleave_expected_sync_offset()
    {
        return _cur_branch ? INTER_MARKER_INTERSAMPS - _cur_branch : 0;
    }

    inline void soft_to_hard(uint8_t *hard, int8_t *soft, int len)
    {
        int i;

        // assert(!(len & 0x7));

        while (len > 0)
        {
            *hard = 0;

            for (i = 7; i >= 0; i--)
            {
                *hard |= (*soft < 0) << i;
                soft++;
            }

            hard++;
            len -= 8;
        }
    }

    DeinterleaverReader::DeinterleaverReader()
    {
    }

    DeinterleaverReader::~DeinterleaverReader()
    {
    }

    int DeinterleaverReader::read_samples(std::function<int(int8_t *, size_t)> read, int8_t *dst, size_t len)
    {
        uint8_t *hard = new uint8_t[INTER_SIZE(len)];

        /* Retrieve enough samples so that the deinterleaver will output
         * $len samples. Use the internal cache first */
        int num_samples = deinterleave_num_samples(len);
        if (offset)
        {
            memcpy(dst, from_prev, MIN(offset, num_samples));
            memcpy(from_prev, from_prev + offset, offset - MIN(offset, num_samples));
        }
        if (num_samples - offset > 0 && !read(dst + offset, num_samples - offset))
        {
            delete[] hard;
            return 1;
        }
        offset -= MIN(offset, num_samples);

        if (num_samples < INTER_MARKER_STRIDE * 8)
        {
            /* Not enough bytes to reliably find sync marker offset: assume the
             * offset is correct, and just derotate and deinterleave what we read */
            // soft_derotate(dst, num_samples, rotation);
            rotate_soft(dst, num_samples, rotation, false);
            deinterleave(dst, dst, len);
        }
        else
        {
            /* Find synchronization marker (offset with the best autocorrelation) */
            soft_to_hard(hard, dst, num_samples & ~0x7);
            offset = autocorrelate(&rotation, INTER_MARKER_STRIDE / 8, hard, num_samples / 8);

            /* Get where the deinterleaver expects the next marker to be */
            int deint_offset = deinterleave_expected_sync_offset();

            /* Compute the delta between the expected marker position and the
             * one found by the correlator */
            offset = (offset - deint_offset + INTER_MARKER_INTERSAMPS + 1) % INTER_MARKER_STRIDE;
            offset = offset > INTER_MARKER_STRIDE / 2 ? offset - INTER_MARKER_STRIDE : offset;

            /* If the offset is positive, read more
             * bits to get $num_samples valid samples. If the offset is negative,
             * copy the last few bytes into the local cache */
            if (offset > 0)
            {
                if (!read(dst + num_samples, offset))
                {
                    delete[] hard;
                    return 1;
                }
            }
            else
            {
                memcpy(from_prev, dst + num_samples + offset, -offset);
            }

            /* Correct rotation for these samples */
            // soft_derotate(dst, num_samples + offset, rotation);
            rotate_soft(dst, num_samples + offset, rotation, false);

            /* Deinterleave */
            deinterleave(dst, dst + offset, len);
            offset = offset < 0 ? -offset : 0;
        }

        delete[] hard;
        return 0;
    }
}
