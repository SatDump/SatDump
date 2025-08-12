// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once


#include "charls_jpegls_decoder.h"
#include "charls_jpegls_encoder.h"
#include "version.h"


// Undefine CHARLS macros to prevent global macro namespace pollution
#if !defined(CHARLS_LIBRARY_BUILD)
#undef CHARLS_API_IMPORT_EXPORT
#undef CHARLS_NO_DISCARD
#undef CHARLS_FINAL
#undef CHARLS_NOEXCEPT
#undef CHARLS_ATTRIBUTE
#undef CHARLS_DEPRECATED
#undef CHARLS_C_VOID
#undef CHARLS_IN
#undef CHARLS_IN_OPT
#undef CHARLS_IN_Z
#undef CHARLS_IN_READS_BYTES
#undef CHARLS_OUT
#undef CHARLS_OUT_OPT
#undef CHARLS_OUT_WRITES_BYTES
#undef CHARLS_OUT_WRITES_Z
#undef CHARLS_RETURN_TYPE_SUCCESS
#undef CHARLS_CHECK_RETURN
#undef CHARLS_RET_MAY_BE_NULL
#undef CHARLS_CONSTEXPR

#endif
