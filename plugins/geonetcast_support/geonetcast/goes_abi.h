#pragma once

#include <vector>
#include "common/image/image.h"

namespace geonetcast
{
    // Is this a massive hack? Yes. Does it work and allow NOT linking libhdf5? Yes!
    image::Image<uint16_t> parse_goesr_abi_netcdf_fulldisk(std::vector<uint8_t> data,
                                                           int side_chunks,
                                                           int bit_depth,
                                                           bool mode2 = false);
}