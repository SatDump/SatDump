#pragma once

#ifdef USE_OPENCL
#define CL_TARGET_OPENCL_VERSION 110
#ifdef __APPLE__
#include <OpenCL/opencl.h>
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

        void setupOCLContext();

        // If the cache is enabled, you should NOT free the kernel
        cl_program buildCLKernel(std::string path, bool use_cache = true);
    }
}
#endif