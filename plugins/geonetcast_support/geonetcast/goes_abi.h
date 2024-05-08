#pragma once

#include <vector>
#include "common/image/image.h"

namespace geonetcast
{
#ifdef ENABLE_HDF5_PARSING
    image::Image parse_goesr_abi_netcdf_fulldisk_CMI(std::vector<uint8_t> data, int bit_depth);
#endif

#if 0
    // Is this a massive hack? Yes. Does it work and allow NOT linking libhdf5? Yes!
    image::Image parse_goesr_abi_netcdf_fulldisk(std::vector<uint8_t> data,
                                                  int side_chunks,
                                                  int bit_depth,
                                                  bool mode2 = false);
#endif
}