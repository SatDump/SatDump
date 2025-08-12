// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause


#include "validate_spiff_header.h"
#include "util.h"

using namespace charls;

namespace charls { namespace {

bool is_valid_color_space(const spiff_color_space color_space, const int32_t component_count) noexcept
{
    switch (color_space)
    {
    case spiff_color_space::none:
        return true;

    case spiff_color_space::bi_level_black:
    case spiff_color_space::bi_level_white:
        return false; // not supported for JPEG-LS.

    case spiff_color_space::grayscale:
        return component_count == 1;

    case spiff_color_space::ycbcr_itu_bt_709_video:
    case spiff_color_space::ycbcr_itu_bt_601_1_rgb:
    case spiff_color_space::ycbcr_itu_bt_601_1_video:
    case spiff_color_space::rgb:
    case spiff_color_space::cmy:
    case spiff_color_space::photo_ycc:
    case spiff_color_space::cie_lab:
        return component_count == 3;

    case spiff_color_space::cmyk:
    case spiff_color_space::ycck:
        return component_count == 4;
    }

    return false;
}

bool is_valid_resolution_units(const spiff_resolution_units resolution_units) noexcept
{
    switch (resolution_units)
    {
    case spiff_resolution_units::aspect_ratio:
    case spiff_resolution_units::dots_per_centimeter:
    case spiff_resolution_units::dots_per_inch:
        return true;
    }

    return false;
}

bool is_valid_spiff_header(const spiff_header& header, const charls::frame_info& frame_info) noexcept
{
    if (header.compression_type != spiff_compression_type::jpeg_ls)
        return false;

    if (header.profile_id != spiff_profile_id::none)
        return false;

    if (!is_valid_resolution_units(header.resolution_units))
        return false;

    if (header.horizontal_resolution == 0 || header.vertical_resolution == 0)
        return false;

    if (header.component_count != frame_info.component_count)
        return false;

    if (!is_valid_color_space(header.color_space, header.component_count))
        return false;

    if (header.bits_per_sample != frame_info.bits_per_sample)
        return false;

    if (header.height != frame_info.height)
        return false;

    if (header.width != frame_info.width)
        return false;

    return true;
}

}} // namespace charls


extern "C" {

USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_validate_spiff_header(const charls_spiff_header* spiff_header, const charls_frame_info* frame_info) noexcept
try
{
    return is_valid_spiff_header(*check_pointer(spiff_header), *check_pointer(frame_info))
               ? jpegls_errc::success
               : jpegls_errc::invalid_spiff_header;
}
catch (...)
{
    return to_jpegls_errc();
}
}
