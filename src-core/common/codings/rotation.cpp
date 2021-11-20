#include "rotation.h"
#include <cmath>

void rotate_soft(int8_t *soft, int size, phase_t phase, bool iqswap)
{
  int8_t tmp;

  // Scale down to avoid overflows
  for (int i = 0; i < size; i++)
    soft[i] = std::max<int>(-127, soft[i]);

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