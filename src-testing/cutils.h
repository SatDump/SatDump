#pragma once

#include <vector>
#include "common/image/image.h"
#include "common/ccsds/ccsds.h"

void calculate_chunking_stategy(int dim_size,
                                int default_num_chunks,
                                int min_chunk_size,
                                int *num_chunks,
                                int *chunk_size);

std::vector<uint8_t> compress_openjpeg(uint16_t *img, int width, int height, int depth, float quality);

std::vector<ccsds::CCSDSPacket> encode_image_chunked(image::Image<uint8_t> source_image, time_t timestamp, int apid, int channel, int maxsize);