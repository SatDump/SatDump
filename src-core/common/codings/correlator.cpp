#include "correlator.h"

uint64_t rotate_64(uint64_t word, uint64_t p)
{
    uint64_t i = word & 0xaaaaaaaaaaaaaaaa;
    uint64_t q = word & 0x5555555555555555;

    switch (p)
    {
    case PHASE_0:
        break;
    case PHASE_90:
        word = ((i ^ 0xaaaaaaaaaaaaaaaa) >> 1) | (q << 1);
        break;
    case PHASE_180:
        word = word ^ 0xffffffffffffffff;
        break;
    case PHASE_270:
        word = (i >> 1) | ((q ^ 0x5555555555555555) << 1);
        break;
    default:
        break;
    }

    return ((word & 0x5555555555555555) << 1) | ((word & 0xAAAAAAAAAAAAAAAA) >> 1);
}

int corr_64(uint64_t v1, uint64_t v2)
{
    int cor = 0;
    uint64_t diff = v1 ^ v2;
    for (; diff; cor++)
        diff &= diff - 1;
    return 64 - cor;
}

uint64_t swapIQ(uint64_t in)
{
    uint64_t i = in & 0xaaaaaaaaaaaaaaaa;
    uint64_t q = in & 0x5555555555555555;
    return (i >> 1) | (q << 1);
}

Correlator::Correlator(constellation_t mod, uint64_t syncword) : d_modulation(mod)
{
    hard_buf = new uint8_t[8192 * 20];

    if (d_modulation == BPSK)
    {
        // Generate syncwords
        syncwords[0] = syncword;
        syncwords[1] = syncword ^ 0xFFFFFFFFFFFFFFFF;
    }
    else if (d_modulation == QPSK)
    {
        // Generate syncwords
        for (int i = 0; i < 4; i++)
            syncwords[i] = rotate_64(syncword, (phase_t)i);

        // Generate rotated syncwords
        for (int i = 4; i < 8; i++)
            syncwords[i] = rotate_64((swapIQ(syncword) ^ 0xFFFFFFFFFFFFFFFF), (phase_t)(i - 4));
    }
}

Correlator::~Correlator()
{
    delete[] hard_buf;
}

int Correlator::correlate(int8_t *soft_input, phase_t &phase, bool &swap, int &cor, int length)
{
    int correlation = 0, offset = 0, pos = 0;

    // Pack into hard symbols
    int bits = 0, bytes = 0;
    uint8_t shifter = 0;
    for (int i = 0; i < length; i++)
    {
        shifter = shifter << 1 | (soft_input[i] >= 0);
        bits++;

        if (bits == 8)
        {
            hard_buf[bytes] = shifter;
            bits = 0;
            bytes++;
        }
    }

    uint64_t current = ((uint64_t)hard_buf[0] << 56) | ((uint64_t)hard_buf[1] << 48) | ((uint64_t)hard_buf[2] << 40) |
                       ((uint64_t)hard_buf[3] << 32) | ((uint64_t)hard_buf[4] << 24) | ((uint64_t)hard_buf[5] << 16) |
                       ((uint64_t)hard_buf[6] << 8) | ((uint64_t)hard_buf[7] << 0);

    if (d_modulation == BPSK)
    {
        pos += 8;

        // Check pos 0
        for (int p = 0; p < 2; p++)
        {
            int corr = corr_64(syncwords[p], current);
            if (corr > 45)
            {
                cor = corr;
                phase = p ? PHASE_180 : PHASE_0;
                swap = 0;
                return 0;
            }
        }

        // Check the rest
        for (int i = 0; i < length - 8; i++)
        {
            for (int ii = 0; ii < 8; ii += 1)
            {
                for (int p = 0; p < 2; p++)
                {
                    int corr = corr_64(syncwords[p], current);
                    if (corr > correlation)
                    {
                        correlation = corr;
                        offset = i * 8 + ii;
                        phase = p ? PHASE_180 : PHASE_0;
                        swap = 0;
                    }
                }

                current = (current << 1) | ((hard_buf[pos] >> (7 - ii)) & 0b1);
            }

            pos++;
        }
    }
    else if (d_modulation == QPSK)
    {
        pos += 8;

        // Check pos 0
        for (int p = 0; p < 8; p++)
        {
            int corr = corr_64(syncwords[p], current);
            if (corr > 45)
            {
                cor = corr;
                phase = (phase_t)(p % 4);
                swap = (p / 4) == 0;
                return 0;
            }
        }

        // Check the rest
        for (int i = 0; i < length - 8; i++)
        {
            for (int ii = 0; ii < 8; ii += 2)
            {
                for (int p = 0; p < 8; p++)
                {
                    int corr = corr_64(syncwords[p], current);
                    if (corr > correlation)
                    {
                        correlation = corr;
                        offset = i * 8 + ii;
                        phase = (phase_t)(p % 4);
                        swap = (p / 4) == 0;
                    }
                }

                current = (current << 2) | ((hard_buf[pos] >> (6 - ii)) & 0b11);
            }

            pos++;
        }
    }

    cor = correlation;
    return offset;
}