// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

namespace charls {

struct coding_parameters final
{
    int32_t near_lossless;
    uint32_t restart_interval;
    charls::interleave_mode interleave_mode;
    color_transformation transformation;
    bool output_bgr;
};

} // namespace charls
