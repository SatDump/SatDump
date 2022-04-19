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
#include "common/image/image.h"

#include "products/image_products.h"
#include "common/tracking/tracking.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "common/geodetic/projection/tps_transform.h"
#include "common/map/map_drawer.h"
#include "resources.h"

#include <iostream>
#include <CL/opencl.hpp>
#include <fstream>
#include "common/utils.h"

#include "common/image/composite.h"

int main(int argc, char *argv[])
{
    initLogger();

    // get all platforms (drivers), e.g. NVIDIA
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);

    if (all_platforms.size() == 0)
    {
        std::cout << " No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Platform default_platform = all_platforms[0];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    // get default device (CPUs, GPUs) of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0)
    {
        std::cout << " No devices found. Check OpenCL installation!\n";
        exit(1);
    }

    // use device[1] because that's a GPU; device[0] is the CPU
    cl::Device default_device = all_devices[0];
    std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

    // a context is like a "runtime link" to the device and platform;
    // i.e. communication is possible
    cl::Context context({default_device});

    // create the program that we want to execute on the device
    cl::Program::Sources sources;

    std::ifstream istream("../src-testing/kernel.cl");
    std::string kernel_src(std::istreambuf_iterator<char>{istream}, {});
    istream.close();

    sources.push_back({kernel_src.c_str(), kernel_src.length()});

    cl::Program program(context, sources);
    if (program.build({default_device}) != CL_SUCCESS)
    {
        std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
        exit(1);
    }

    satdump::ImageProducts img_pro;
    img_pro.load("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_3/AVHRR/product.cbor");

    // ThinPlateSpline::PointList input_points;
    // ThinPlateSpline::PointList output_points;

    std::vector<geodetic::projection::GCP> gcps;

    satdump::SatelliteTracker sat_tracker(img_pro.get_tle());

    image::Image<uint16_t> img_map(2048 * 8, 1024 * 8, 3);

    std::vector<int> values;

    for (int x = 0; x < 2048; x += 100)
        values.push_back(x);

    values.push_back(2047);

    int y = 0;
    for (double timestamp : img_pro.contents["timestamps"].get<std::vector<double>>())
    {
        // timestamp -= 0.15;

        geodetic::geodetic_coords_t pos_curr = sat_tracker.get_sat_position_at(timestamp);     // Current position
        geodetic::geodetic_coords_t pos_next = sat_tracker.get_sat_position_at(timestamp + 1); // Upcoming position

        double az_angle = vincentys_inverse(pos_next, pos_curr).reverse_azimuth * RAD_TO_DEG;

        for (int x : values)
        {
            //  if (y % 200 == 0)
            //      logger->info(x);
            bool invert_scan = true;

            float roll_offset = -0.13;
            float pitch_offset = 0;
            float yaw_offset = 0;
            float scan_angle = 110.6;

            double final_x = invert_scan ? (2048 - 1) - x : x;
            bool ascending = false;

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((final_x - (2048.0 / 2.0)) / 2048.0) * scan_angle) + roll_offset;
            satellite_pointing.pitch = pitch_offset;
            satellite_pointing.yaw = (90 + (ascending ? yaw_offset : -yaw_offset)) - az_angle;

            geodetic::geodetic_coords_t ground_position;
            geodetic::raytrace_to_earth(pos_curr, satellite_pointing, ground_position);
            ground_position.toDegs();

            // logger->info("{:d} {:d} {:f} {:s}", x, y, pos_curr.lat, img_pro.get_tle().name);

            if (y % 100 == 0 || y + 1 == img_pro.contents["timestamps"].get<std::vector<double>>().size())
            {
                gcps.push_back({x, y, ground_position.lon, ground_position.lat});
                //  output_points.push_back(Eigen::Vector3d(x, y, 0));
                //  input_points.push_back(Eigen::Vector3d(image_x, image_y, 0));
            }
        }

        // logger->info("{:d}", y);

        y++;
    }

    logger->info(gcps.size());

    logger->info("Solving...");
    // ThinPlateSpline tps(input_points, output_points);
    // tps.solve();
    geodetic::projection::TPSTransform tps(gcps);
    logger->info("Done!");

    int p_lat_min = -90;
    int p_lat_max = 90;
    int p_lon_min = -180;
    int p_lon_max = 180;

    int p_y_min = 0;
    int p_y_max = img_map.height();
    int p_x_min = 0;
    int p_x_max = img_map.width();

    // Determine min/max lat/lon we have to cover to warp this image properly
    {
        std::vector<double> lat_values;
        std::vector<double> lon_values;
        for (geodetic::projection::GCP g : gcps)
        {
            lat_values.push_back(g.lat);
            lon_values.push_back(g.lon);
        }

        double lat_min = 0;
        double lat_max = 0;
        double lon_min = 0;
        double lon_max = 0;
        lat_min = lat_max = avg_overflowless(lat_values);
        lon_min = lon_max = avg_overflowless(lon_values);

        for (geodetic::projection::GCP g : gcps)
        {
            if (g.lat > lat_max)
                lat_max = g.lat;
            if (g.lat < lat_min)
                lat_min = g.lat;

            if (g.lon > lon_max)
                lon_max = g.lon;
            if (g.lon < lon_min)
                lon_min = g.lon;
        }

        p_lat_min = floor(lat_min);
        p_lon_min = floor(lon_min);
        p_lat_max = ceil(lat_max);
        p_lon_max = ceil(lon_max);

        logger->info("Lat min {:d}", p_lat_min);
        logger->info("Lat max {:d}", p_lat_max);
        logger->info("Lon min {:d}", p_lon_min);
        logger->info("Lon max {:d}", p_lon_max);

        // int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
        //                                int imageLon = (lon / 360.0f) * map_width + (map_width / 2);

        p_y_max = img_map.height() - ((90.0f + p_lat_min) / 180.0f) * img_map.height();
        p_y_min = img_map.height() - ((90.0f + p_lat_max) / 180.0f) * img_map.height();
        p_x_min = (p_lon_min / 360.0f) * img_map.width() + (img_map.width() / 2);
        p_x_max = (p_lon_max / 360.0f) * img_map.width() + (img_map.width() / 2);

        logger->info("Y min {:d}", p_y_min);
        logger->info("Y max {:d}", p_y_max);
        logger->info("X min {:d}", p_x_min);
        logger->info("X max {:d}", p_x_max);
    }

    image::Image<uint16_t> img = image::generate_composite_from_equ<uint16_t>({std::get<2>(img_pro.images[0]), std::get<2>(img_pro.images[1]), std::get<2>(img_pro.images[2])}, {"1", "2", "3"}, "(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15", "");

    time_t gpu_start = time(0);
    {
        cl::Buffer buffer_map(context, CL_MEM_READ_WRITE, sizeof(uint16_t) * img_map.size());
        logger->info(img_map.size());
        cl::Buffer buffer_img(context, CL_MEM_READ_WRITE, sizeof(uint16_t) * img.size());

        cl::Buffer buffer_tps_npoints(context, CL_MEM_READ_WRITE, sizeof(int));
        cl::Buffer buffer_tps_x(context, CL_MEM_READ_WRITE, sizeof(double) * tps.getRawForward()._nof_points);
        cl::Buffer buffer_tps_y(context, CL_MEM_READ_WRITE, sizeof(double) * tps.getRawForward()._nof_points);
        cl::Buffer buffer_tps_coefs1(context, CL_MEM_READ_WRITE, sizeof(double) * tps.getRawForward()._nof_eqs);
        cl::Buffer buffer_tps_coefs2(context, CL_MEM_READ_WRITE, sizeof(double) * tps.getRawForward()._nof_eqs);

        cl::Buffer buffer_tps_xmean(context, CL_MEM_READ_WRITE, sizeof(double));
        cl::Buffer buffer_tps_ymean(context, CL_MEM_READ_WRITE, sizeof(double));

        int img_settings[] = {img_map.width(), img_map.height(), img.width(), img.height(), img_map.channels(), p_y_min, p_y_max, p_x_min, p_x_max};

        cl::Buffer buffer_img_settings(context, CL_MEM_READ_WRITE, sizeof(int) * 9);

        // create a queue (a queue of commands that the GPU will execute)
        cl::CommandQueue queue(context, default_device);

        // push write commands to queue
        queue.enqueueWriteBuffer(buffer_map, CL_TRUE, 0, sizeof(uint16_t) * img_map.size(), img_map.data());
        queue.enqueueWriteBuffer(buffer_img, CL_TRUE, 0, sizeof(uint16_t) * img.size(), img.data());

        queue.enqueueWriteBuffer(buffer_tps_npoints, CL_TRUE, 0, sizeof(int), &tps.getRawForward()._nof_points);
        queue.enqueueWriteBuffer(buffer_tps_x, CL_TRUE, 0, sizeof(double) * tps.getRawForward()._nof_points, tps.getRawForward().x);
        queue.enqueueWriteBuffer(buffer_tps_y, CL_TRUE, 0, sizeof(double) * tps.getRawForward()._nof_points, tps.getRawForward().y);
        queue.enqueueWriteBuffer(buffer_tps_coefs1, CL_TRUE, 0, sizeof(double) * tps.getRawForward()._nof_eqs, tps.getRawForward().coef[0]);
        queue.enqueueWriteBuffer(buffer_tps_coefs2, CL_TRUE, 0, sizeof(double) * tps.getRawForward()._nof_eqs, tps.getRawForward().coef[1]);
        queue.enqueueWriteBuffer(buffer_tps_xmean, CL_TRUE, 0, sizeof(double), &tps.getRawForward().x_mean);
        queue.enqueueWriteBuffer(buffer_tps_ymean, CL_TRUE, 0, sizeof(double), &tps.getRawForward().y_mean);
        queue.enqueueWriteBuffer(buffer_img_settings, CL_TRUE, 0, sizeof(int) * 9, img_settings);

        logger->critical(tps.getRawForward()._nof_eqs);

        // RUN ZE KERNEL
        cl::Kernel simple_add(program, "warp_image_thin_plate_spline");
        simple_add.setArg(0, buffer_map);
        simple_add.setArg(1, buffer_img);

        simple_add.setArg(2, buffer_tps_npoints);
        simple_add.setArg(3, buffer_tps_x);
        simple_add.setArg(4, buffer_tps_y);
        simple_add.setArg(5, buffer_tps_coefs1);
        simple_add.setArg(6, buffer_tps_coefs2);
        simple_add.setArg(7, buffer_tps_xmean);
        simple_add.setArg(8, buffer_tps_ymean);
        simple_add.setArg(9, buffer_img_settings);

        logger->info("Start GPU");
        queue.enqueueNDRangeKernel(simple_add, cl::NullRange, cl::NDRange(1920), cl::NullRange);
        // logger->info("Stop GPU");

        // read result from GPU to here
        queue.enqueueReadBuffer(buffer_map, CL_TRUE, 0, sizeof(uint16_t) * img_map.size(), img_map.data());
        logger->info("GPU Done");

        logger->info(img_map[0]);
    }
    time_t gpu_time = time(0) - gpu_start;
    logger->info("GPU Time {:d}", gpu_time);

    /*
        time_t cpu_start = time(0);
        double xx, yy;

        for (int y = 0; y < img_map.height(); y++)
        {
            for (int x = 0; x < img_map.width(); x++)
            {
                // Scale to the map
                double lat = -(double(y) / double(img_map.height())) * 180 + 90;
                double lon = (double(x) / double(img_map.width())) * 360 - 180;

                 tps.forward(lon, lat, xx, yy);
                //  logger->info("{:f} {:f}", xx, yy);

                if (xx < 0 || yy < 0)
                    continue;

                if (xx > img1.width() - 1 || yy > img1.height() - 1)
                    continue;

                // for (int c = 0; c < 3; c++)
                img_map.channel(0)[y * img_map.width() + x] = img2.channel(0)[int(yy) * img1.width() + int(xx)];
                img_map.channel(1)[y * img_map.width() + x] = img2.channel(0)[int(yy) * img1.width() + int(xx)];
                img_map.channel(2)[y * img_map.width() + x] = img1.channel(0)[int(yy) * img1.width() + int(xx)];
                //  img_map.channel(1)[y * img1.width() + x] = img1.channel(1)[(int)floor(yy) * img1.width() + (int)floor(xx)];
                //  img_map.channel(2)[y * img1.width() + x] = img1.channel(2)[(int)floor(yy) * img1.width() + (int)floor(xx)];
            }

            logger->info("{:d}", y);
        }
        time_t cpu_time = time(0) - cpu_start;
        logger->info("CPU Time {:d}", cpu_time);
    */

    unsigned short color[3] = {0, 65535, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   img_map,
                                   color,
                                   [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
                                   {
                                       int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
                                       int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
                                       return {imageLon, imageLat};
                                   });

    img_map.save_png("test.png");
}