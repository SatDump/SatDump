/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "init.h"

#include "image/image.h"
#include "common/tracking/tracking.h"
#include "common/tracking/tle.h"

#include "common/projection/projs/equirectangular.h"

#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"

#include "products/image_products.h"

int main(int argc, char *argv[])
{
    initLogger();

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatDump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    if (argc < 9)
    {
        logger->error("Usage ./imager_simulator norad start_timestamp_unix stop_timestamp_unix scan_period_seconds scan_angle scan_size output_folder input_background.png");
        return 1;
    }

    int norad = std::stoi(argv[1]);
    double start_time = std::stod(argv[2]);
    double stop_time = std::stod(argv[3]);
    double scan_period = std::stod(argv[4]);
    double scan_angle = std::stod(argv[5]);
    int scan_size = std::stod(argv[6]);
    std::string output_folder = argv[7];

    int scan_count = (stop_time - start_time) / scan_period;

    logger->info("Simulating data for NORAD %d", norad);
    logger->info("Start Time %f", start_time);
    logger->info("Stop Time %f", stop_time);
    logger->info("Duration %f", stop_time - start_time);
    logger->info("Scan period %f", scan_period);
    logger->info("Scan count %d", scan_count);
    logger->info("Scan angle %f", scan_angle);
    logger->info("Scan size %d", scan_size);
    logger->info("Scan angle size %f", scan_angle / scan_size);

    logger->debug("Loading source image...");

    image::Image<uint8_t> input_equirect;
    input_equirect.load_png(argv[8]);

    geodetic::projection::EquirectangularProjection equ_projector;
    equ_projector.init(input_equirect.width(), input_equirect.height(), -180, 90, 180, -90);

    logger->debug("Start generation...");

    auto tles = satdump::db_tle->get_from_norad(norad).value();
    satdump::SatelliteTracker sat_tracker(tles);

    logger->trace(tles.name);

    bool invert_scan = false;

    image::Image<uint16_t> image_r(scan_size, scan_count, 1);
    image::Image<uint16_t> image_g(scan_size, scan_count, 1);
    image::Image<uint16_t> image_b(scan_size, scan_count, 1);

    int y = 0;

    std::vector<double> timestamps;

    for (double timestamp = start_time; timestamp < stop_time && y < scan_count; timestamp += scan_period)
    {
        geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
        geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position
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
                logger->error("Invalid earth position! %f %f - %f %f %f - %f %f",
                              ground_position.lon, ground_position.lat,
                              satellite_pointing.roll,
                              satellite_pointing.pitch,
                              satellite_pointing.yaw,
                              pos_curr.lon, pos_curr.lat);
                return 1;
            }

            int src_pos_x, src_pos_y;
            equ_projector.forward(ground_position.lon, ground_position.lat, src_pos_x, src_pos_y);

            image_r[y * scan_size + x] = input_equirect.channel(0)[src_pos_y * input_equirect.width() + src_pos_x] << 8;
            image_g[y * scan_size + x] = input_equirect.channel(1)[src_pos_y * input_equirect.width() + src_pos_x] << 8;
            image_b[y * scan_size + x] = input_equirect.channel(2)[src_pos_y * input_equirect.width() + src_pos_x] << 8;
        }

        timestamps.push_back(timestamp);

        y++;

        logger->trace("Line %d/%d", y, scan_count);
    }

    satdump::ImageProducts imager_products;
    imager_products.instrument_name = "simulated_imager";
    imager_products.has_timestamps = true;
    imager_products.bit_depth = 16;
    imager_products.set_tle(tles);
    imager_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
    imager_products.set_timestamps(timestamps);

    nlohmann::ordered_json proj_settings;

    proj_settings["type"] = "normal_single_line";
    proj_settings["scan_angle"] = scan_angle;
    proj_settings["image_width"] = scan_size;
    proj_settings["gcp_spacing_x"] = 100;
    proj_settings["gcp_spacing_y"] = 100;

    imager_products.set_proj_cfg(proj_settings);

    imager_products.images.push_back({"IMAGER-1", "1", image_r});
    imager_products.images.push_back({"IMAGER-2", "2", image_g});
    imager_products.images.push_back({"IMAGER-3", "3", image_b});

    if (!std::filesystem::exists(output_folder))
        std::filesystem::create_directories(output_folder);

    imager_products.save(output_folder);

    satdump::exitSatDump();
}
