#pragma once

#include "common/dsp/complex.h"
#include "nlohmann/json.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(complex_t, real, imag)