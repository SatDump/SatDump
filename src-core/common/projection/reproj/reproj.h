#pragma once

#include "common/image/image.h"

namespace satdump
{
    namespace reproj
    {
        void reproject_equ_to_stereo(
            image::Image<uint16_t> &source_img, // Source image
            float equ_tl_lon, float equ_tl_lat, // Top-Left corner
            float equ_br_lon, float equ_br_lat, // Bottom-Right corner
            image::Image<uint16_t> &target_img, // Target image
            float ste_center_lat,               // Stereo center lat
            float ste_center_lon,               // Stereo center lon
            float ste_scale,                    // Stereo Scale
            float *progress);

        void reproject_equ_to_tpers(
            image::Image<uint16_t> &source_img, // Source image
            float equ_tl_lon, float equ_tl_lat, // Top-Left corner
            float equ_br_lon, float equ_br_lat, // Bottom-Right corner
            image::Image<uint16_t> &target_img, // Target image
            float tpe_altitude,                 // TPERS Alt
            float tpe_longitude,                // TPERS Lon
            float tpe_latitude,                 // TPERS Lat
            float tpe_tilt,                     // TPERS Tilt
            float tpe_azi,                      // TPERS Azi
            float *progress);

        void reproject_geos_to_equ(
            image::Image<uint16_t> &source_img, // Source image
            double geos_lon,
            double geos_height,
            double geos_hscale,
            double geos_vscale,
            double geos_xoff,
            double geos_yoff,
            bool geos_sweepx,
            image::Image<uint16_t> &target_img, // Target image
            float equ_tl_lon, float equ_tl_lat, // Top-Left corner
            float equ_br_lon, float equ_br_lat, // Bottom-Right corner
            float *progress);

        void reproject_merc_to_equ(
            image::Image<uint16_t> &source_img, // Source image
                                                // IDK yet
            image::Image<uint16_t> &target_img, // Target image
            float equ_tl_lon, float equ_tl_lat, // Top-Left corner
            float equ_br_lon, float equ_br_lat, // Bottom-Right corner
            float *progress);

        void reproject_equ_to_azeq(image::Image<uint16_t> &source_img, // source image
                                   float equ_tl_lon, float equ_tl_lat, // Top-left conrner
                                   float equ_br_lon, float equ_br_lat, // Bottom-left corner
                                   image::Image<uint16_t> &target_img, // target image
                                   float azeq_longitude,               // Azeq center longitude
                                   float azeq_latitude,                // Azeq center latitude
                                   float *progress);
    }
}