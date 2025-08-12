// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

namespace charls {

// JPEG Marker codes have the pattern 0xFFaa in a JPEG byte stream.
// The valid 'aa' options are defined by several ISO/IEC, ITU standards:
// 0x00, 0x01, 0xFE, 0xC0-0xDF are defined in ISO/IEC 10918-1, ITU T.81
// 0xF0 - 0xF6 are defined in ISO/IEC 10918-3 | ITU T.84: JPEG extensions
// 0xF7 - 0xF8 are defined in ISO/IEC 14495-1 | ITU T.87: JPEG LS baseline
// 0xF9         is defined in ISO/IEC 14495-2 | ITU T.870: JPEG LS extensions
// 0x4F - 0x6F, 0x90 - 0x93 are defined in ISO/IEC 15444-1: JPEG 2000

constexpr uint8_t jpeg_marker_start_byte{0xFF};
constexpr uint8_t jpeg_restart_marker_base{0xD0}; // RSTm: Marks the next restart interval (range is D0..D7)
constexpr uint32_t jpeg_restart_marker_range{8};

enum class jpeg_marker_code : uint8_t
{
    start_of_image = 0xD8,          // SOI: Marks the start of an image.
    end_of_image = 0xD9,            // EOI: Marks the end of an image.
    start_of_scan = 0xDA,           // SOS: Marks the start of scan.
    define_restart_interval = 0xDD, // DRI: Defines the restart interval used in succeeding scans.

    // The following markers are defined in ISO/IEC 10918-1 | ITU T.81.
    start_of_frame_baseline_jpeg = 0xC0,       // SOF_0:  Marks the start of a baseline jpeg encoded frame.
    start_of_frame_extended_sequential = 0xC1, // SOF_1:  Marks the start of a extended sequential Huffman encoded frame.
    start_of_frame_progressive = 0xC2,         // SOF_2:  Marks the start of a progressive Huffman encoded frame.
    start_of_frame_lossless = 0xC3,            // SOF_3:  Marks the start of a lossless Huffman encoded frame.
    start_of_frame_differential_sequential =
        0xC5, // SOF_5:  Marks the start of a differential sequential Huffman encoded frame.
    start_of_frame_differential_progressive =
        0xC6, // SOF_6:  Marks the start of a differential progressive Huffman encoded frame.
    start_of_frame_differential_lossless = 0xC7, // SOF_7:  Marks the start of a differential lossless Huffman encoded frame.
    start_of_frame_extended_arithmetic = 0xC9, // SOF_9:  Marks the start of a extended sequential arithmetic encoded frame.
    start_of_frame_progressive_arithmetic = 0xCA, // SOF_10: Marks the start of a progressive arithmetic encoded frame.
    start_of_frame_lossless_arithmetic = 0xCB,    // SOF_11: Marks the start of a lossless arithmetic encoded frame.

    // The following markers are defined in ISO/IEC 14495-1 | ITU T.87.
    start_of_frame_jpegls = 0xF7,          // SOF_55: Marks the start of a JPEG-LS encoded frame.
    jpegls_preset_parameters = 0xF8,       // LSE:    Marks the start of a JPEG-LS preset parameters segment.
    start_of_frame_jpegls_extended = 0xF9, // SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

    application_data0 = 0xE0,  // APP0:  Application data 0: used for JFIF header.
    application_data1 = 0xE1,  // APP1:  Application data 1: used for EXIF or XMP header.
    application_data2 = 0xE2,  // APP2:  Application data 2: used for ICC profile.
    application_data3 = 0xE3,  // APP3:  Application data 3: used for meta info
    application_data4 = 0xE4,  // APP4:  Application data 4.
    application_data5 = 0xE5,  // APP5:  Application data 5.
    application_data6 = 0xE6,  // APP6:  Application data 6.
    application_data7 = 0xE7,  // APP7:  Application data 7: used for HP color-space info.
    application_data8 = 0xE8,  // APP8:  Application data 8: used for HP color-transformation info or SPIFF header.
    application_data9 = 0xE9,  // APP9:  Application data 9.
    application_data10 = 0xEA, // APP10: Application data 10.
    application_data11 = 0xEB, // APP11: Application data 11.
    application_data12 = 0xEC, // APP12: Application data 12: used for Picture info.
    application_data13 = 0xED, // APP13: Application data 13: used by PhotoShop IRB
    application_data14 = 0xEE, // APP14: Application data 14: used by Adobe
    application_data15 = 0xEF, // APP15: Application data 15.
    comment = 0xFE             // COM:   Comment block.
};

} // namespace charls
