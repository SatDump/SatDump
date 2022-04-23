#include "warp.h"
#include "logger.h"
#include <map>
#include "common/utils.h"
#include "resources.h"

namespace satdump
{
#ifdef USE_OPENCL
    namespace opencl
    {
        cl::Program buildCLKernel(cl::Context context, cl::Device device, std::string path)
        {
            cl::Program::Sources sources;
            std::ifstream isf(path);
            std::string kernel_src(std::istreambuf_iterator<char>{isf}, {});
            sources.push_back({kernel_src.c_str(), kernel_src.length()});

            cl::Program program(context, sources);
            if (program.build({device}) != CL_SUCCESS)
            {
                logger->error("Error building: {:s}", program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
                return program;
            }

            return program;
        }

        std::pair<cl::Context, cl::Device> getDeviceAndContext(int platform, int device)
        {
            std::vector<cl::Platform> all_platforms;
            cl::Platform::get(&all_platforms);

            if (all_platforms.size() == 0)
                std::runtime_error("No platforms found. Check OpenCL installation!");

            cl::Platform default_platform = all_platforms[platform];
            logger->info("Using platform: {:s}", default_platform.getInfo<CL_PLATFORM_NAME>());

            // get default device (CPUs, GPUs) of the default platform
            std::vector<cl::Device> all_devices;
            default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
            if (all_devices.size() == 0)
                std::runtime_error("No devices found. Check OpenCL installation!");

            cl::Device default_device = all_devices[device];
            logger->info("Using device: {:s}", default_device.getInfo<CL_DEVICE_NAME>());

            cl::Context context({default_device});

            return std::pair<cl::Context, cl::Device>(context, default_device);
        }
    };
#endif

    namespace warp
    {
        std::unique_ptr<projection::VizGeorefSpline2D> initTPSTransform(WarpOperation &op)
        {
            std::unique_ptr<projection::VizGeorefSpline2D> spline_transform = std::make_unique<projection::VizGeorefSpline2D>(2);

            std::vector<projection::GCP> gcps = op.ground_control_points;

            // Attach (non-redundant) points to the transformation.
            std::map<std::pair<double, double>, int> oMapPixelLineToIdx;
            std::map<std::pair<double, double>, int> oMapXYToIdx;
            for (int iGCP = 0; iGCP < (int)gcps.size(); iGCP++)
            {
                const double afPL[2] = {gcps[iGCP].x, gcps[iGCP].y};
                const double afXY[2] = {gcps[iGCP].lon, gcps[iGCP].lat};

                std::map<std::pair<double, double>, int>::iterator oIter(oMapPixelLineToIdx.find(std::pair<double, double>(afPL[0], afPL[1])));

                if (oIter != oMapPixelLineToIdx.end())
                {
                    if (afXY[0] == gcps[oIter->second].lon && afXY[1] == gcps[oIter->second].lat)
                        continue;
                    else
                        logger->warn("2 GCPs have the same X,Y!");
                }
                else
                    oMapPixelLineToIdx[std::pair<double, double>(afPL[0], afPL[1])] = iGCP;

                if (oMapXYToIdx.find(std::pair<double, double>(afXY[0], afXY[1])) != oMapXYToIdx.end())
                    logger->warn("2 GCPs have the same Lat,Lon!");
                else
                    oMapXYToIdx[std::pair<double, double>(afXY[0], afXY[1])] = iGCP;

                if (!spline_transform->add_point(afXY[0], afXY[1], afPL))
                {
                    logger->error("Error generating transformer!");
                    // return 1;
                }
            }

            logger->info("Solving TPS equations for {:d} GCPs...", gcps.size());
            auto solve_start = std::chrono::system_clock::now();
            bool solved = spline_transform->solve() != 0;
            if (solved)
                logger->info("Solved! Took {:f}", (std::chrono::system_clock::now() - solve_start).count() / 1e9);
            else
                logger->error("Failure solving!");

            return spline_transform;
        }

        WarpCropSettings choseCropArea(WarpOperation &op)
        {
            WarpCropSettings cset;
            cset.lat_min = -90;
            cset.lat_max = 90;
            cset.lon_min = -180;
            cset.lon_max = 180;
            cset.y_min = 0;
            cset.y_max = op.output_height;
            cset.x_min = 0;
            cset.x_max = op.output_width;

            std::vector<double> lat_values;
            std::vector<double> lon_values;
            for (projection::GCP g : op.ground_control_points)
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

            for (projection::GCP g : op.ground_control_points)
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

            // Round to integer degrees
            cset.lat_min = floor(lat_min);
            cset.lon_min = floor(lon_min);
            cset.lat_max = ceil(lat_max);
            cset.lon_max = ceil(lon_max);

            // Compute to pixels
            cset.y_max = op.output_height - ((90.0f + cset.lat_min) / 180.0f) * op.output_height;
            cset.y_min = op.output_height - ((90.0f + cset.lat_max) / 180.0f) * op.output_height;
            cset.x_min = (cset.lon_min / 360.0f) * op.output_width + (op.output_width / 2);
            cset.x_max = (cset.lon_max / 360.0f) * op.output_width + (op.output_width / 2);

            // Pixels can offset it a bit - recompute to be 100% accurate
            cset.lat_max = ((op.output_height - cset.y_min) / (double)op.output_height) * 180.0f - 90.0f;
            cset.lat_min = ((op.output_height - cset.y_max) / (double)op.output_height) * 180.0f - 90.0f;
            cset.lon_min = (cset.x_min / (double)op.output_width) * 360.0f - 180.0f;
            cset.lon_max = (cset.x_max / (double)op.output_width) * 360.0f - 180.0f;

            return cset;
        }

        void ImageWarper::warpOnCPU(WarpResult &result)
        {
            // Now, warp the image
            auto cpu_start = std::chrono::system_clock::now();
            {
                double xx, yy;
                double xy[2];
                for (size_t xy_ptr = 0; xy_ptr < result.output_image.size(); xy_ptr++)
                {
                    int x = (xy_ptr % result.output_image.width());
                    int y = (xy_ptr / result.output_image.width());

                    // Scale to the map
                    double lat = -((double)(y + crop_set.y_min) / (double)op.output_height) * 180 + 90;
                    double lon = ((double)(x + crop_set.x_min) / (double)op.output_width) * 360 - 180;

                    // Perform TPS
                    tps->get_point(lon, lat, xy);
                    xx = xy[0];
                    yy = xy[1];

                    if (xx < 0 || yy < 0)
                        continue;

                    if ((int)xx > op.input_image.width() - 1 || (int)yy > op.input_image.height() - 1)
                        continue;

                    for (int c = 0; c < op.input_image.channels(); c++)
                        result.output_image.channel(c)[y * result.output_image.width() + x] = op.input_image.channel(c)[(int)yy * op.input_image.width() + (int)xx];
                }
            }
            auto cpu_time = (std::chrono::system_clock::now() - cpu_start);
            logger->debug("CPU Processing Time {:f}", cpu_time.count() / 1e9);
        }

#ifdef USE_OPENCL
        void ImageWarper::warpOnGPU_fp64(cl::Context context, cl::Device device, WarpResult &result)
        {
            // Build GPU Kernel
            cl::Program warping_program = opencl::buildCLKernel(context, device, resources::getResourcePath("opencl/warp_image_thin_plate_spline_fp64.cl"));

            // Now, run the actual OpenCL Kernel
            auto gpu_start = std::chrono::system_clock::now();
            {
                // Images
                cl::Buffer buffer_map(context, CL_MEM_READ_WRITE, sizeof(uint16_t) * result.output_image.size());
                cl::Buffer buffer_img(context, CL_MEM_READ_WRITE, sizeof(uint16_t) * op.input_image.size());

                // TPS Stuff
                cl::Buffer buffer_tps_npoints(context, CL_MEM_READ_WRITE, sizeof(int));
                cl::Buffer buffer_tps_x(context, CL_MEM_READ_WRITE, sizeof(double) * tps->_nof_points);
                cl::Buffer buffer_tps_y(context, CL_MEM_READ_WRITE, sizeof(double) * tps->_nof_points);
                cl::Buffer buffer_tps_coefs1(context, CL_MEM_READ_WRITE, sizeof(double) * tps->_nof_eqs);
                cl::Buffer buffer_tps_coefs2(context, CL_MEM_READ_WRITE, sizeof(double) * tps->_nof_eqs);
                cl::Buffer buffer_tps_xmean(context, CL_MEM_READ_WRITE, sizeof(double));
                cl::Buffer buffer_tps_ymean(context, CL_MEM_READ_WRITE, sizeof(double));

                int img_settings[] = {op.output_width, op.output_height,
                                      (int)op.input_image.width(), (int)op.input_image.height(),
                                      op.input_image.channels(),
                                      crop_set.y_min, crop_set.y_max,
                                      crop_set.x_min, crop_set.x_max};

                cl::Buffer buffer_img_settings(context, CL_MEM_READ_WRITE, sizeof(int) * 9);

                // Create an OpenCL queue
                cl::CommandQueue queue(context, device);

                // Write all of buffers to the GPU
                queue.enqueueWriteBuffer(buffer_map, CL_TRUE, 0, sizeof(uint16_t) * result.output_image.size(), result.output_image.data());
                queue.enqueueWriteBuffer(buffer_img, CL_TRUE, 0, sizeof(uint16_t) * op.input_image.size(), op.input_image.data());
                queue.enqueueWriteBuffer(buffer_tps_npoints, CL_TRUE, 0, sizeof(int), &tps->_nof_points);
                queue.enqueueWriteBuffer(buffer_tps_x, CL_TRUE, 0, sizeof(double) * tps->_nof_points, tps->x);
                queue.enqueueWriteBuffer(buffer_tps_y, CL_TRUE, 0, sizeof(double) * tps->_nof_points, tps->y);
                queue.enqueueWriteBuffer(buffer_tps_coefs1, CL_TRUE, 0, sizeof(double) * tps->_nof_eqs, tps->coef[0]);
                queue.enqueueWriteBuffer(buffer_tps_coefs2, CL_TRUE, 0, sizeof(double) * tps->_nof_eqs, tps->coef[1]);
                queue.enqueueWriteBuffer(buffer_tps_xmean, CL_TRUE, 0, sizeof(double), &tps->x_mean);
                queue.enqueueWriteBuffer(buffer_tps_ymean, CL_TRUE, 0, sizeof(double), &tps->y_mean);
                queue.enqueueWriteBuffer(buffer_img_settings, CL_TRUE, 0, sizeof(int) * 9, img_settings);

                // Init the kernel
                cl::Kernel warping_kernel(warping_program, "warp_image_thin_plate_spline");
                warping_kernel.setArg(0, buffer_map);
                warping_kernel.setArg(1, buffer_img);
                warping_kernel.setArg(2, buffer_tps_npoints);
                warping_kernel.setArg(3, buffer_tps_x);
                warping_kernel.setArg(4, buffer_tps_y);
                warping_kernel.setArg(5, buffer_tps_coefs1);
                warping_kernel.setArg(6, buffer_tps_coefs2);
                warping_kernel.setArg(7, buffer_tps_xmean);
                warping_kernel.setArg(8, buffer_tps_ymean);
                warping_kernel.setArg(9, buffer_img_settings);

                // Get proper workload size
                cl_uint size_wg;
                cl_uint compute_units;

                {
                    size_t wg = 0;
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_WORK_GROUP_SIZE, 0, NULL, &wg);
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_WORK_GROUP_SIZE, wg, &size_wg, NULL);
                }

                {
                    size_t wg = 0;
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_COMPUTE_UNITS, 0, NULL, &wg);
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_COMPUTE_UNITS, wg, &compute_units, NULL);
                }

                logger->debug("Workgroup size {:d}", size_wg * compute_units);

                // Run the kernel!
                queue.enqueueNDRangeKernel(warping_kernel, cl::NullRange, cl::NDRange(int(size_wg) * int(compute_units)), cl::NullRange);

                // Read image result back from VRAM
                queue.enqueueReadBuffer(buffer_map, CL_TRUE, 0, sizeof(uint16_t) * result.output_image.size(), result.output_image.data());
            }
            auto gpu_time = (std::chrono::system_clock::now() - gpu_start);
            logger->debug("GPU Processing Time {:f}", gpu_time.count() / 1e9);
        }

        void ImageWarper::warpOnGPU_fp32(cl::Context context, cl::Device device, WarpResult &result)
        {
            // Build GPU Kernel
            cl::Program warping_program = opencl::buildCLKernel(context, device, resources::getResourcePath("opencl/warp_image_thin_plate_spline_fp32.cl"));

            // Now, run the actual OpenCL Kernel
            auto gpu_start = std::chrono::system_clock::now();
            {
                // Images
                cl::Buffer buffer_map(context, CL_MEM_READ_WRITE, sizeof(uint16_t) * result.output_image.size());
                cl::Buffer buffer_img(context, CL_MEM_READ_WRITE, sizeof(uint16_t) * op.input_image.size());

                // TPS Stuff
                cl::Buffer buffer_tps_npoints(context, CL_MEM_READ_WRITE, sizeof(int));
                cl::Buffer buffer_tps_x(context, CL_MEM_READ_WRITE, sizeof(float) * tps->_nof_points);
                cl::Buffer buffer_tps_y(context, CL_MEM_READ_WRITE, sizeof(float) * tps->_nof_points);
                cl::Buffer buffer_tps_coefs1(context, CL_MEM_READ_WRITE, sizeof(float) * tps->_nof_eqs);
                cl::Buffer buffer_tps_coefs2(context, CL_MEM_READ_WRITE, sizeof(float) * tps->_nof_eqs);
                cl::Buffer buffer_tps_xmean(context, CL_MEM_READ_WRITE, sizeof(float));
                cl::Buffer buffer_tps_ymean(context, CL_MEM_READ_WRITE, sizeof(float));

                int img_settings[] = {op.output_width, op.output_height,
                                      (int)op.input_image.width(), (int)op.input_image.height(),
                                      op.input_image.channels(),
                                      crop_set.y_min, crop_set.y_max,
                                      crop_set.x_min, crop_set.x_max};

                cl::Buffer buffer_img_settings(context, CL_MEM_READ_WRITE, sizeof(int) * 9);

                // Create an OpenCL queue
                cl::CommandQueue queue(context, device);

                // Write all of buffers to the GPU, also converting to FP32
                queue.enqueueWriteBuffer(buffer_map, CL_TRUE, 0, sizeof(uint16_t) * result.output_image.size(), result.output_image.data());
                queue.enqueueWriteBuffer(buffer_img, CL_TRUE, 0, sizeof(uint16_t) * op.input_image.size(), op.input_image.data());
                queue.enqueueWriteBuffer(buffer_tps_npoints, CL_TRUE, 0, sizeof(int), &tps->_nof_points);
                std::vector<float> tps_x = double_buffer_to_float(tps->x, tps->_nof_points);
                std::vector<float> tps_y = double_buffer_to_float(tps->y, tps->_nof_points);
                std::vector<float> tps_coef1 = double_buffer_to_float(tps->coef[0], tps->_nof_eqs);
                std::vector<float> tps_coef2 = double_buffer_to_float(tps->coef[1], tps->_nof_eqs);
                float tps_x_mean = tps->x_mean;
                float tps_y_mean = tps->y_mean;
                queue.enqueueWriteBuffer(buffer_tps_x, CL_TRUE, 0, sizeof(float) * tps->_nof_points, tps_x.data());
                queue.enqueueWriteBuffer(buffer_tps_y, CL_TRUE, 0, sizeof(float) * tps->_nof_points, tps_y.data());
                queue.enqueueWriteBuffer(buffer_tps_coefs1, CL_TRUE, 0, sizeof(float) * tps->_nof_eqs, tps_coef1.data());
                queue.enqueueWriteBuffer(buffer_tps_coefs2, CL_TRUE, 0, sizeof(float) * tps->_nof_eqs, tps_coef2.data());
                queue.enqueueWriteBuffer(buffer_tps_xmean, CL_TRUE, 0, sizeof(float), &tps_x_mean);
                queue.enqueueWriteBuffer(buffer_tps_ymean, CL_TRUE, 0, sizeof(float), &tps_y_mean);
                queue.enqueueWriteBuffer(buffer_img_settings, CL_TRUE, 0, sizeof(int) * 9, img_settings);

                // Init the kernel
                cl::Kernel warping_kernel(warping_program, "warp_image_thin_plate_spline");
                warping_kernel.setArg(0, buffer_map);
                warping_kernel.setArg(1, buffer_img);
                warping_kernel.setArg(2, buffer_tps_npoints);
                warping_kernel.setArg(3, buffer_tps_x);
                warping_kernel.setArg(4, buffer_tps_y);
                warping_kernel.setArg(5, buffer_tps_coefs1);
                warping_kernel.setArg(6, buffer_tps_coefs2);
                warping_kernel.setArg(7, buffer_tps_xmean);
                warping_kernel.setArg(8, buffer_tps_ymean);
                warping_kernel.setArg(9, buffer_img_settings);

                // Get proper workload size
                cl_uint size_wg;
                cl_uint compute_units;

                {
                    size_t wg = 0;
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_WORK_GROUP_SIZE, 0, NULL, &wg);
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_WORK_GROUP_SIZE, wg, &size_wg, NULL);
                }

                {
                    size_t wg = 0;
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_COMPUTE_UNITS, 0, NULL, &wg);
                    clGetDeviceInfo(device.get(), CL_DEVICE_MAX_COMPUTE_UNITS, wg, &compute_units, NULL);
                }

                logger->debug("Workgroup size {:d}", size_wg * compute_units);

                // Run the kernel!
                queue.enqueueNDRangeKernel(warping_kernel, cl::NullRange, cl::NDRange(int(size_wg) * int(compute_units)), cl::NullRange);

                // Read image result back from VRAM
                queue.enqueueReadBuffer(buffer_map, CL_TRUE, 0, sizeof(uint16_t) * result.output_image.size(), result.output_image.data());
            }
            auto gpu_time = (std::chrono::system_clock::now() - gpu_start);
            logger->debug("GPU Processing Time {:f}", gpu_time.count() / 1e9);
        }
#endif

        void ImageWarper::update()
        {
            tps = initTPSTransform(op);
            crop_set = choseCropArea(op);
        }

        WarpResult ImageWarper::warp()
        {
            WarpResult result;

            // Prepare the output
            result.output_image = image::Image<uint16_t>(crop_set.x_max - crop_set.x_min, crop_set.y_max - crop_set.y_min, op.input_image.channels());
            result.top_left = {0, 0, (double)crop_set.lon_min, (double)crop_set.lat_max};                                                                                  // 0,0
            result.top_right = {(double)result.output_image.width() - 1, 0, (double)crop_set.lon_max, (double)crop_set.lat_max};                                           // 1,0
            result.bottom_left = {0, (double)result.output_image.height() - 1, (double)crop_set.lon_min, (double)crop_set.lat_min};                                        // 0,1
            result.bottom_right = {(double)result.output_image.width() - 1, (double)result.output_image.height() - 1, (double)crop_set.lon_max, (double)crop_set.lat_min}; // 1,1

#ifdef USE_OPENCL
            try
            {
                logger->debug("Using GPU!");
                std::pair<cl::Context, cl::Device> opencl_context = opencl::getDeviceAndContext(0, 0);
                warpOnGPU_fp32(opencl_context.first, opencl_context.second, result);
                return result;
            }
            catch (std::runtime_error &e)
            {
                logger->error("Error warping on GPU : {:s}", e.what());
            }
            catch (cl::Error &e)
            {
                logger->error("OpenCL error warping on GPU : {:s}", e.what());
            }
#endif

            logger->debug("Using CPU!");
            warpOnCPU(result);

            return result;
        }
    }
}