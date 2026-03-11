#pragma once

/**
 * @file complex_json.h
 * @brief Convert complet_t to/from nlohmann::json
 */

#include "common/dsp/complex.h"
#include "nlohmann/json.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(complex_t, real, imag)