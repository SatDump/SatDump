// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

namespace charls {

enum class jpegls_preset_parameters_type : uint8_t
{
    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): Preset coding parameters (defined in C.2.4.1.1).
    /// </summary>
    preset_coding_parameters = 0x1,

    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table specification (defined in C.2.4.1.2).
    /// </summary>
    mapping_table_specification = 0x2,

    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table continuation (defined in C.2.4.1.3).
    /// </summary>
    mapping_table_continuation = 0x3,

    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): X and Y parameters are defined (defined in C.2.4.1.4).
    /// </summary>
    oversize_image_dimension = 0x4,

    // The values below are from JPEG-LS Extended (ISO/IEC 14495-2) standard and defined only for better error reporting.

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): Coding method specification.
    /// </summary>
    coding_method_specification = 0x5,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): NEAR value re-specification.
    /// </summary>
    near_lossless_error_re_specification = 0x6,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): Visually oriented quantization specification.
    /// </summary>
    visually_oriented_quantization_specification = 0x7,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): Extended prediction specification.
    /// </summary>
    extended_prediction_specification = 0x8,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): Specification of the start of fixed length coding.
    /// </summary>
    start_of_fixed_length_coding = 0x9,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): Specification of the end of fixed length coding.
    /// </summary>
    end_of_fixed_length_coding = 0xA,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): JPEG-LS preset coding parameters.
    /// </summary>
    extended_preset_coding_parameters = 0xC,

    /// <summary>
    /// JPEG-LS Extended (ISO/IEC 14495-2): inverse color transform specification.
    /// </summary>
    inverse_color_transform_specification = 0xD
};

} // namespace charls
