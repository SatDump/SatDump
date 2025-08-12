// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
/// Validates a SPIFF header. It is validated against the ISO/IEC 10918-3, Annex F and if the
/// values match with the frame info argument.
/// </summary>
/// <remarks>
/// ISO/IEC 10918-3 (SPIFF) and ISO/IEC 14495-1 (JPEG-LS) use different allowed ranges for
/// bits per sample: (1, 2, 4, 8, 12, 16) vs [2..16]. The JPEG-LS range is used for validation.
/// </remarks>
/// <param name="spiff_header">Reference to a charls_spiff_header instance.</param>
/// <param name="frame_info">Reference to a charls_frame_info instance.</param>
/// <returns>charls_jpegls_errc::success when the SPIFF header is valid, otherwise charls_jpegls_errc::invalid_spiff_header.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_validate_spiff_header(
    CHARLS_IN const charls_spiff_header* spiff_header, CHARLS_IN const charls_frame_info* frame_info) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

#ifdef __cplusplus
} // extern "C"
#endif
