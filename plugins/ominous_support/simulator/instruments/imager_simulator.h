#pragma once

#include "common/tracking/tracking.h"
#include "image/image.h"
#include "image/io.h"
#include "logger.h"

#include "projection/utils/equirectangular.h"

#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"

class ImagerSimulator
{
private:
    const double scan_angle;
    const int scan_size;

    satdump::projection::EquirectangularProjection equ_projector;

    std::shared_ptr<satdump::SatelliteTracker> sat_tracker;

    image::Image input_equirect;

public:
    ImagerSimulator(std::shared_ptr<satdump::SatelliteTracker> sat_tracker, double scan_angle, int scan_size, std::string img_path)
        : sat_tracker(sat_tracker), scan_angle(scan_angle), scan_size(scan_size)
    {
        // logger->info("Simulating data for NORAD {:d}", norad);
        logger->info("Scan angle {:f}", scan_angle);
        logger->info("Scan size {:d}", scan_size);
        logger->info("Scan angle size {:f}", scan_angle / scan_size);

        logger->debug("Loading source image...");
        image::load_img(input_equirect, img_path);
        input_equirect = input_equirect.to16bits();
        equ_projector.init(input_equirect.width(), input_equirect.height(), -180, 90, 180, -90);
    }

    ~ImagerSimulator() {}

    int get_scanline(double timestamp, uint16_t *scan_r, uint16_t *scan_g, uint16_t *scan_b)
    {
        bool invert_scan = false;

        geodetic::geodetic_coords_t pos_curr = sat_tracker->get_sat_position_at(timestamp);     // Current position
        geodetic::geodetic_coords_t pos_next = sat_tracker->get_sat_position_at(timestamp + 1); // Upcoming position
        double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

        for (int x = 0; x < scan_size; x++)
        {
            double final_x = !invert_scan ? (scan_size - 1) - x : x;

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((final_x - (scan_size / 2.0)) / scan_size) * scan_angle);
            satellite_pointing.pitch = 0;
            satellite_pointing.yaw = 90 - az_angle;

            geodetic::geodetic_coords_t ground_position;
            int ret = geodetic::raytrace_to_earth_old(pos_curr, satellite_pointing, ground_position);
            ground_position.toDegs();

            if (ret)
            {
                // logger->error("Invalid earth position! {:f} {:f} - {:f} {:f} {:f} - {:f} {:f}",
                //               ground_position.lon, ground_position.lat,
                //               satellite_pointing.roll,
                //               satellite_pointing.pitch,
                //               satellite_pointing.yaw,
                //               pos_curr.lon, pos_curr.lat);
                continue;
            }

            int src_pos_x, src_pos_y;
            equ_projector.forward(ground_position.lon, ground_position.lat, src_pos_x, src_pos_y);

            if (src_pos_y > (int)input_equirect.height() || src_pos_x > (int)input_equirect.width() || src_pos_y < 0 || src_pos_x < 0)
                continue;

            scan_r[x] = input_equirect.get_pixel_bilinear(0, src_pos_x, src_pos_y);
            scan_g[x] = input_equirect.get_pixel_bilinear(1, src_pos_x, src_pos_y);
            scan_b[x] = input_equirect.get_pixel_bilinear(2, src_pos_x, src_pos_y);
        }

        return 0;
    }

    int get_picture(double timestamp, double fov_x, double fov_y, double roll, double pitch, double raw, int width, int height, uint16_t *scan_r, uint16_t *scan_g, uint16_t *scan_b)
    {
        bool invert_scan = false;

        geodetic::geodetic_coords_t pos_curr = sat_tracker->get_sat_position_at(timestamp);     // Current position
        geodetic::geodetic_coords_t pos_next = sat_tracker->get_sat_position_at(timestamp + 1); // Upcoming position
        double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                double final_x = !invert_scan ? (width - 1) - x : x;

                geodetic::euler_coords_t satellite_pointing;
                satellite_pointing.roll = roll - (((final_x - (width / 2.0)) / width) * fov_x);
                satellite_pointing.pitch = pitch + (((y - (height / 2.0)) / height) * fov_y);
                satellite_pointing.yaw = raw + 90 - az_angle;

                geodetic::geodetic_coords_t ground_position;
                int ret = geodetic::raytrace_to_earth_old(pos_curr, satellite_pointing, ground_position);
                ground_position.toDegs();

                if (ret)
                {
                    // logger->error("Invalid earth position! {:f} {:f} - {:f} {:f} {:f} - {:f} {:f}",
                    //               ground_position.lon, ground_position.lat,
                    //               satellite_pointing.roll,
                    //               satellite_pointing.pitch,
                    //               satellite_pointing.yaw,
                    //               pos_curr.lon, pos_curr.lat);
                    continue;
                }

                int src_pos_x, src_pos_y;
                equ_projector.forward(ground_position.lon, ground_position.lat, src_pos_x, src_pos_y);

                if (src_pos_y > (int)input_equirect.height() || src_pos_x > (int)input_equirect.width() || src_pos_y < 0 || src_pos_x < 0)
                    continue;

                scan_r[y * width + x] = input_equirect.get(0, src_pos_x, src_pos_y);
                scan_g[y * width + x] = input_equirect.get(1, src_pos_x, src_pos_y);
                scan_b[y * width + x] = input_equirect.get(2, src_pos_x, src_pos_y);
            }
        }

        return 0;
    }
};