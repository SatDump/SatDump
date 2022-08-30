#include "opencl.h"
#include "logger.h"
#include "core/config.h"
#include <fstream>

#ifdef USE_OPENCL
namespace satdump
{
    namespace opencl
    {
        std::vector<OCLDevice> getAllDevices()
        {
            std::vector<OCLDevice> devs;
            std::vector<cl::Platform> all_platforms;

            try
            {
                cl::Platform::get(&all_platforms);
            }
            catch (cl::Error &e)
            {
                logger->error("OpenCL Error : {:s}", e.what());
                return devs;
            }

            for (int p = 0; p < (int)all_platforms.size(); p++)
            {
                std::vector<cl::Device> all_devices;
                all_platforms[p].getDevices(CL_DEVICE_TYPE_ALL, &all_devices);

                for (int d = 0; d < (int)all_devices.size(); d++)
                    devs.push_back({p, d, all_devices[d].getInfo<CL_DEVICE_NAME>()});
            }

            return devs;
        }

        void initOpenCL()
        {
            std::vector<OCLDevice> devices = getAllDevices();
            logger->info("Found OpenCL Devices ({:d}) :", devices.size());
            for (OCLDevice &d : devices)
                logger->debug(" - " + d.name);
        }

        bool context_is_init = false;
        cl::Context ocl_context;
        cl::Device ocl_device;

        void setupOCLContext()
        {
            int platform_id = satdump::config::main_cfg["satdump_general"]["opencl_device"]["platform"].get<int>();
            int device_id = satdump::config::main_cfg["satdump_general"]["opencl_device"]["device"].get<int>();

            if (!context_is_init)
            {
                logger->trace("First OpenCL context request. Initializing...");

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

                ocl_device = all_devices[device_id];
                logger->info("Using device: {:s}", ocl_device.getInfo<CL_DEVICE_NAME>());

                ocl_context = cl::Context({ocl_device});

                context_is_init = true;
            }
            else
            {
                logger->trace("OpenCL context already initilized.");
            }
        }

        cl::Program buildCLKernel(std::string path)
        {
            cl::Program::Sources sources;
            std::ifstream isf(path);
            std::string kernel_src(std::istreambuf_iterator<char>{isf}, {});
            sources.push_back({kernel_src.c_str(), kernel_src.length()});

            cl::Program program(ocl_context, sources);
            try
            {
                program.build({ocl_device});
            }
            catch (cl::BuildError &e)
            {
                logger->error("Error building: {:s}", program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(ocl_device).c_str());
                return program;
            }

            return program;
        }
    }
}
#endif