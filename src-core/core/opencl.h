#pragma once

#ifdef USE_OPENCL
#define CL_TARGET_OPENCL_VERSION 110
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#elif defined(__ANDROID__)
#include "libs/libopencl_loader/opencl_loader.h"
#include "libs/libopencl_loader/CL/cl.hpp"
#else
#include <CL/cl.h>
#endif
#include <string>
#include <vector>

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

        extern cl_context ocl_context;
        extern cl_device_id ocl_device;

        bool useCL();
        void setupOCLContext();
        std::vector<OCLDevice> resetOCLContext();
        // If the cache is enabled, you should NOT free the kernel
        cl_program buildCLKernel(std::string path, bool use_cache = true);
    }
}
#endif
