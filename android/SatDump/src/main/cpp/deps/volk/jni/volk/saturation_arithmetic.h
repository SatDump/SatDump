/* -*- c++ -*- */
/*
 * Copyright 2016 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_volk_saturation_arithmetic_H_
#define INCLUDED_volk_saturation_arithmetic_H_

#include <limits.h>

static inline int16_t sat_adds16i(int16_t x, int16_t y)
{
    int32_t res = (int32_t)x + (int32_t)y;

    if (res < SHRT_MIN)
        res = SHRT_MIN;
    if (res > SHRT_MAX)
        res = SHRT_MAX;

    return res;
}

static inline int16_t sat_muls16i(int16_t x, int16_t y)
{
    int32_t res = (int32_t)x * (int32_t)y;

    if (res < SHRT_MIN)
        res = SHRT_MIN;
    if (res > SHRT_MAX)
        res = SHRT_MAX;

    return res;
}

#endif /* INCLUDED_volk_saturation_arithmetic_H_ */
