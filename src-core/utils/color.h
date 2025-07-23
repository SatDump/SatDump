#pragma once

/**
 * @file color.h
 * @brief Color manipulation utilities
 */

#include <cstdint>

namespace satdump
{
    /**
     * @brief Convert HSV floats to 8-bit RGB
     *
     * @param h H Value
     * @param s S Value
     * @param v V Value
     * @param rgb output RGB buffer
     */
    void hsv_to_rgb(float h, float s, float v, uint8_t *rgb);
} // namespace satdump