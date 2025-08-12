// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include "util.h"

#include <cassert>
#include <cstdint>

namespace charls {

/// <summary>
/// JPEG-LS uses arrays of variables: A[0..366], B[0..364], C[0..364] and N[0..366]
/// to maintain the statistic information for the context modeling.
/// Index 365 and 366 are used for run mode interruption contexts.
/// </summary>
class context_run_mode final
{
public:
    context_run_mode() = default;

    context_run_mode(const int32_t run_interruption_type, const int32_t range) noexcept :
        run_interruption_type_{run_interruption_type}, a_{initialization_value_for_a(range)}
    {
    }

    int32_t run_interruption_type() const noexcept
    {
        return run_interruption_type_;
    }

    FORCE_INLINE int32_t get_golomb_code() const noexcept
    {
        const int32_t temp{a_ + (n_ >> 1) * run_interruption_type_};
        int32_t n_test{n_};
        int32_t k{};
        for (; n_test < temp; ++k)
        {
            n_test <<= 1;
            ASSERT(k <= 32);
        }
        return k;
    }

    /// <summary>Code segment A.23 – Update of variables for run interruption sample.</summary>
    void update_variables(const int32_t error_value, const int32_t e_mapped_error_value,
                          const uint8_t reset_threshold) noexcept
    {
        if (error_value < 0)
        {
            ++nn_;
        }

        a_ += (e_mapped_error_value + 1 - run_interruption_type_) >> 1;

        if (n_ == reset_threshold)
        {
            a_ >>= 1;
            n_ = static_cast<uint8_t>(n_ >> 1);
            nn_ = static_cast<uint8_t>(nn_ >> 1);
        }

        ++n_;
    }

    FORCE_INLINE int32_t compute_error_value(const int32_t temp, const int32_t k) const noexcept
    {
        const bool map = temp & 1;
        const int32_t error_value_abs{(temp + static_cast<int32_t>(map)) / 2};

        if ((k != 0 || (2 * nn_ >= n_)) == map)
        {
            ASSERT(map == compute_map(-error_value_abs, k));
            return -error_value_abs;
        }

        ASSERT(map == compute_map(error_value_abs, k));
        return error_value_abs;
    }

    /// <summary>Code segment A.21 – Computation of map for Errval mapping.</summary>
    bool compute_map(const int32_t error_value, const int32_t k) const noexcept
    {
        if (k == 0 && error_value > 0 && 2 * nn_ < n_)
            return true;

        if (error_value < 0 && 2 * nn_ >= n_)
            return true;

        if (error_value < 0 && k != 0)
            return true;

        return false;
    }

    FORCE_INLINE bool compute_map_negative_e(const int32_t k) const noexcept
    {
        return k != 0 || 2 * nn_ >= n_;
    }

private:
    // Initialize with the default values as defined in ISO 14495-1, A.8, step 1.d and 1.f.
    int32_t run_interruption_type_{};
    int32_t a_{};
    uint8_t n_{1};
    uint8_t nn_{};
};

} // namespace charls
