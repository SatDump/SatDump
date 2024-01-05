#include "rotation.h"
#include <cmath>

void rotate_soft(int8_t *soft, int size, phase_t phase, bool iqswap)
{
    int8_t tmp;

    // Scale down to avoid overflows
    for (int i = 0; i < size; i++)
        if (soft[i] == -128)
            soft[i] = -127;

    // Swap I & Q if requested
    if (iqswap)
    {
        for (int i = 0; i < size; i += 2)
        {
            int8_t x = soft[i + 1];
            soft[i + 1] = soft[i];
            soft[i] = x;
        }
    }

    // Rotate phase
    switch (phase)
    {
    case PHASE_0:
        // Nothing to do!
        break;
    case PHASE_90:
        // Rotate 90 deg
        for (; size > 0; size -= 2)
        {
            tmp = *soft;
            *soft = *(soft + 1);
            *(soft + 1) = -tmp;
            soft += 2;
        }
        break;
    case PHASE_180:
        // Rotate 180
        for (; size > 0; size--)
        {
            *soft = -*soft;
            soft++;
        }
        break;
    case PHASE_270:
        // Rotate 270
        for (; size > 0; size -= 2)
        {
            tmp = *soft;
            *soft = -*(soft + 1);
            *(soft + 1) = tmp;
            soft += 2;
        }
        break;

    default:
        // This should never happen
        break;
    }
}

int8_t clamp(float x)
{
    if (x < -128.0)
        return -127;
    if (x > 127.0)
        return 127;
    return x;
}

void rotate_soft_arbitrary(int8_t *soft, int size, float phase)
{
    float shift_real = cos(phase);
    float shift_imag = sin(phase);

    int8_t vr = 0;
    int8_t vi = 0;

    for (int i = 0; i < size / 2; i++)
    {
        vr = clamp((soft[i * 2 + 0] * shift_real) - (soft[i * 2 + 1] * shift_imag));
        vi = clamp((soft[i * 2 + 1] * shift_real) + (soft[i * 2 + 0] * shift_imag));
        soft[i * 2 + 0] = vr;
        soft[i * 2 + 1] = vi;
    }
}