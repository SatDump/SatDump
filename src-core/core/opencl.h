#pragma once

#ifdef USE_OPENCL
#define CL_HPP_MINIMUM_OPENCL_VERSION 110 // Support down to 1.1
#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include "libs/opencl/opencl.hpp"
#else
#include <CL/opencl.hpp>
#endif

namespace satdump
{
    namespace opencl
    {
        struct OCLDevice
        {
            int platform_id;
            int device_id;
            std::string name;
        };

        std::vector<OCLDevice> getAllDevices();

        void initOpenCL();

        extern cl::Context ocl_context;
        extern cl::Device ocl_device;

        void setupOCLContext();

        cl::Program buildCLKernel(std::string path);
    }
}
#endif