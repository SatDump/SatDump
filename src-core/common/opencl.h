#pragma once

#include "logger.h"
#include <fstream>

#ifdef USE_OPENCL
#define CL_HPP_MINIMUM_OPENCL_VERSION 110 // Support down to 1.1
#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include "common/opencl.hpp"
#else
#include <CL/opencl.hpp>
#endif

namespace opencl
{
    inline cl::Program buildCLKernel(cl::Context context, cl::Device device, std::string path)
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

    inline std::pair<cl::Context, cl::Device> getDeviceAndContext(int platform_id, int device_id)
    {
        std::vector<cl::Platform> all_platforms;
        cl::Platform::get(&all_platforms);

        if (all_platforms.size() == 0)
            std::runtime_error("No platforms found. Check OpenCL installation!");

        cl::Platform platform = all_platforms[platform_id];
        logger->info("Using platform: {:s}", platform.getInfo<CL_PLATFORM_NAME>());

        // get default device (CPUs, GPUs) of the default platform
        std::vector<cl::Device> all_devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
        if (all_devices.size() == 0)
            std::runtime_error("No devices found. Check OpenCL installation!");

        cl::Device device = all_devices[device_id];
        logger->info("Using device: {:s}", device.getInfo<CL_DEVICE_NAME>());

        cl::Context context({device});

        return std::pair<cl::Context, cl::Device>(context, device);
    }
};
#endif
