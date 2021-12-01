#pragma once

#include <mutex>
#include "types.h"

// Rotate soft symbols, limited to a few phase states but faster
void rotate_soft(int8_t *soft, int size, phase_t phase, bool iqswap);

// Perform ANY rotation on soft symbols, but this is slightly slower
void rotate_soft_arbitrary(int8_t *soft, int size, float phase);