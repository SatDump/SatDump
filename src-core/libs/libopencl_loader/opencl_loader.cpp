//
//  opencl_loader.cpp
//  libOpenCL_loader
//
//  Created by Guohui Wang on 4/23/18.
//  Copyright © 2019 Guohui Wang. All rights reserved.
//

#ifdef __ANDROID__
#include "opencl_loader.h"
#include "opencl_header_private.h"

#include <iostream>
#include <memory>

#ifdef __ANDROID_API__
#include <android/log.h>
#define CL_LOADER_LOG(...) \
    __android_log_print(ANDROID_LOG_VERBOSE, "libopencl_loader", __VA_ARGS__)
#define CL_LOADER_LOGD(...) \
    __android_log_print(ANDROID_LOG_DEBUG, "libopencl_loader", __VA_ARGS__)
#define CL_LOADER_LOGW(...) \
    __android_log_print(ANDROID_LOG_WARN, "libopencl_loader", __VA_ARGS__)
#define CL_LOADER_LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, "libopencl_loader", __VA_ARGS__)
#else
#define CL_LOADER_LOG(...) printf(__VA_ARGS__)
#define CL_LOADER_LOGD(...) printf(__VA_ARGS__)
#define CL_LOADER_LOGW(...) printf(__VA_ARGS__)
#define CL_LOADER_LOGE(...) printf(__VA_ARGS__)
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define WIN64_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#define strdup _strdup
typedef HMODULE CL_LOADER_DYNLIB_HANDLE;
#define CL_LOADER_DLOPEN LoadLibraryA
#define CL_LOADER_CLCLOSE FreeLibrary
#define CL_LOADER_DLSYM GetProcAddress
#else
#include <dlfcn.h>
typedef void* CL_LOADER_DYNLIB_HANDLE;
#define CL_LOADER_DLOPEN(LIB) dlopen(LIB, RTLD_LAZY)
#define CL_LOADER_CLCLOSE dlclose
#define CL_LOADER_DLSYM dlsym
#endif

static CL_LOADER_DYNLIB_HANDLE handle = nullptr;
#define REINTERPRET_CAST_FUNC(fn) \
    fn = reinterpret_cast<fn##_func>(CL_LOADER_DLSYM(libraryHandle, #fn));

namespace OpenCLHelper {
namespace {
/* Declare local function stubs to be casted to the opencl functions at runtime
 */
using clBuildProgram_func = cl_int (*)(cl_program,
                                       cl_uint,
                                       const cl_device_id*,
                                       const char*,
                                       void (*pfn_notify)(cl_program, void*),
                                       void*);
using clEnqueueNDRangeKernel_func = cl_int (*)(cl_command_queue,
                                               cl_kernel,
                                               cl_uint,
                                               const size_t*,
                                               const size_t*,
                                               const size_t*,
                                               cl_uint,
                                               const cl_event*,
                                               cl_event*);
using clSetKernelArg_func = cl_int (*)(cl_kernel, cl_uint, size_t, const void*);
using clReleaseMemObject_func = cl_int (*)(cl_mem);
using clEnqueueUnmapMemObject_func = cl_int (*)(cl_command_queue,
                                                cl_mem,
                                                void*,
                                                cl_uint,
                                                const cl_event*,
                                                cl_event*);
using clRetainCommandQueue_func = cl_int (*)(cl_command_queue command_queue);
using clReleaseContext_func = cl_int (*)(cl_context);
using clReleaseEvent_func = cl_int (*)(cl_event);
using clEnqueueWriteBuffer_func = cl_int (*)(cl_command_queue,
                                             cl_mem,
                                             cl_bool,
                                             size_t,
                                             size_t,
                                             const void*,
                                             cl_uint,
                                             const cl_event*,
                                             cl_event*);
using clEnqueueReadBuffer_func = cl_int (*)(cl_command_queue,
                                            cl_mem,
                                            cl_bool,
                                            size_t,
                                            size_t,
                                            void*,
                                            cl_uint,
                                            const cl_event*,
                                            cl_event*);
using clGetProgramBuildInfo_func = cl_int (*)(cl_program,
                                              cl_device_id,
                                              cl_program_build_info,
                                              size_t,
                                              void*,
                                              size_t*);
using clRetainProgram_func = cl_int (*)(cl_program program);
using clEnqueueMapBuffer_func = void* (*) (cl_command_queue,
                                           cl_mem,
                                           cl_bool,
                                           cl_map_flags,
                                           size_t,
                                           size_t,
                                           cl_uint,
                                           const cl_event*,
                                           cl_event*,
                                           cl_int*);
using clReleaseCommandQueue_func = cl_int (*)(cl_command_queue);
using clCreateProgramWithBinary_func = cl_program (*)(cl_context,
                                                      cl_uint,
                                                      const cl_device_id*,
                                                      const size_t*,
                                                      const unsigned char**,
                                                      cl_int*,
                                                      cl_int*);
using clRetainContext_func = cl_int (*)(cl_context context);
using clReleaseProgram_func = cl_int (*)(cl_program program);
using clFlush_func = cl_int (*)(cl_command_queue command_queue);
using clGetProgramInfo_func =
    cl_int (*)(cl_program, cl_program_info, size_t, void*, size_t*);
using clGetSupportedImageFormats_func = cl_int (*)(cl_context context,
                                                   cl_mem_flags,
                                                   cl_mem_object_type,
                                                   cl_uint,
                                                   cl_image_format*,
                                                   cl_uint*);
using clCreateKernel_func = cl_kernel (*)(cl_program, const char*, cl_int*);
using clRetainKernel_func = cl_int (*)(cl_kernel kernel);
using clCreateBuffer_func =
    cl_mem (*)(cl_context, cl_mem_flags, size_t, void*, cl_int*);
using clCreateProgramWithSource_func =
    cl_program (*)(cl_context, cl_uint, const char**, const size_t*, cl_int*);
using clReleaseKernel_func = cl_int (*)(cl_kernel);
using clCreateCommandQueue_func =
    cl_command_queue (*)(cl_context,
                         cl_device_id,
                         cl_command_queue_properties,
                         cl_int*);
using clCreateContextFromType_func = cl_context (*)(
    const cl_context_properties*,
    cl_device_type,
    void(CL_CALLBACK* /* pfn_notify*/)(const char*, const void*, size_t, void*),
    void*,
    cl_int*);
using clGetContextInfo_func =
    cl_int (*)(cl_context, cl_context_info, size_t, void*, size_t*);
using clGetDeviceInfo_func =
    cl_int (*)(cl_device_id, cl_device_info, size_t, void*, size_t*);
using clGetPlatformIDs_func = cl_int (*)(cl_uint, cl_platform_id*, cl_uint*);
using clGetPlatformInfo_func =
    cl_int (*)(cl_platform_id, cl_platform_info, size_t, void*, size_t*);
using clRetainDevice_func = cl_int (*)(cl_device_id);
using clReleaseDevice_func = cl_int (*)(cl_device_id);
using clCreateContext_func =
    cl_context (*)(const cl_context_properties*,
                   cl_uint,
                   const cl_device_id*,
                   void(CL_CALLBACK*)(const char*, const void*, size_t, void*),
                   void*,
                   cl_int*);
using clFinish_func = cl_int (*)(cl_command_queue);
using clGetDeviceIDs_func = cl_int (*)(cl_platform_id,
                                       cl_device_type,
                                       cl_uint,
                                       cl_device_id*,
                                       cl_uint*);
using clEnqueueFillBuffer_func = cl_int (*)(cl_command_queue,
                                            cl_mem,
                                            const void*,
                                            size_t,
                                            size_t,
                                            size_t,
                                            cl_uint,
                                            const cl_event*,
                                            cl_event*);
using clCreateImage_func = cl_mem (*)(cl_context,
                                      cl_mem_flags,
                                      const cl_image_format*,
                                      const cl_image_desc*,
                                      void*,
                                      cl_int*);
using clEnqueueReadImage_func =
    cl_int (*)(cl_command_queue /* command_queue */,
               cl_mem /* image */,
               cl_bool /* blocking_read */,
               const size_t* /* origin[3] */,
               const size_t* /* region[3] */,
               size_t /* row_pitch */,
               size_t /* slice_pitch */,
               void* /* ptr */,
               cl_uint /* num_events_in_wait_list */,
               const cl_event* /* event_wait_list */,
               cl_event* /* event */);
using clEnqueueWriteImage_func =
    cl_int (*)(cl_command_queue /* command_queue */,
               cl_mem /* image */,
               cl_bool /* blocking_write */,
               const size_t* /* origin[3] */,
               const size_t* /* region[3] */,
               size_t /* input_row_pitch */,
               size_t /* input_slice_pitch */,
               const void* /* ptr */,
               cl_uint /* num_events_in_wait_list */,
               const cl_event* /* event_wait_list */,
               cl_event* /* event */);
using clEnqueueMapImage_func = void* (*) (cl_command_queue,
                                          cl_mem,
                                          cl_bool,
                                          cl_map_flags,
                                          const size_t*,
                                          const size_t*,
                                          size_t*,
                                          size_t*,
                                          cl_uint,
                                          const cl_event*,
                                          cl_event*,
                                          cl_int*);
using clGetKernelWorkGroupInfo_func = cl_int (*)(cl_kernel,
                                                 cl_device_id,
                                                 cl_kernel_work_group_info,
                                                 size_t,
                                                 void*,
                                                 size_t*);
using clWaitForEvents_func = cl_int (*)(cl_uint, const cl_event*);
using clGetEventProfilingInfo_func =
    cl_int (*)(cl_event, cl_profiling_info, size_t, void*, size_t*);

using clCreateImage2D_func = cl_mem (*)(cl_context,
                                        cl_mem_flags,
                                        const cl_image_format*,
                                        size_t,
                                        size_t,
                                        size_t,
                                        void*,
                                        cl_int*);
using clRetainMemObject_func = cl_int (*)(cl_mem);
using clRetainEvent_func = cl_int (*)(cl_event);

class CLSymbols
{
 public:
    static std::shared_ptr<CLSymbols> symbolsInstance;

    static CLSymbols& Get()
    {
        if (!CLSymbols::symbolsInstance) {
            CLSymbols::symbolsInstance =
                std::shared_ptr<CLSymbols>(new CLSymbols(handle));
        }
        return *CLSymbols::symbolsInstance;
    }

    static void Reset()
    {
        CLSymbols::symbolsInstance = nullptr;
    }

    clBuildProgram_func clBuildProgram = nullptr;
    clEnqueueNDRangeKernel_func clEnqueueNDRangeKernel = nullptr;
    clSetKernelArg_func clSetKernelArg = nullptr;
    clReleaseKernel_func clReleaseKernel = nullptr;
    clCreateProgramWithSource_func clCreateProgramWithSource = nullptr;
    clCreateBuffer_func clCreateBuffer = nullptr;
    clRetainKernel_func clRetainKernel = nullptr;
    clCreateKernel_func clCreateKernel = nullptr;
    clGetProgramInfo_func clGetProgramInfo = nullptr;
    clFlush_func clFlush = nullptr;
    clReleaseProgram_func clReleaseProgram = nullptr;
    clRetainContext_func clRetainContext = nullptr;
    clCreateProgramWithBinary_func clCreateProgramWithBinary = nullptr;
    clReleaseCommandQueue_func clReleaseCommandQueue = nullptr;
    clEnqueueMapBuffer_func clEnqueueMapBuffer = nullptr;
    clRetainProgram_func clRetainProgram = nullptr;
    clGetProgramBuildInfo_func clGetProgramBuildInfo = nullptr;
    clEnqueueReadBuffer_func clEnqueueReadBuffer = nullptr;
    clEnqueueWriteBuffer_func clEnqueueWriteBuffer = nullptr;
    clReleaseEvent_func clReleaseEvent = nullptr;
    clReleaseContext_func clReleaseContext = nullptr;
    clRetainCommandQueue_func clRetainCommandQueue = nullptr;
    clEnqueueUnmapMemObject_func clEnqueueUnmapMemObject = nullptr;
    clReleaseMemObject_func clReleaseMemObject = nullptr;
    clCreateCommandQueue_func clCreateCommandQueue = nullptr;
    clCreateContextFromType_func clCreateContextFromType = nullptr;
    clGetContextInfo_func clGetContextInfo = nullptr;
    clGetDeviceInfo_func clGetDeviceInfo = nullptr;
    clGetSupportedImageFormats_func clGetSupportedImageFormats = nullptr;
    clGetPlatformIDs_func clGetPlatformIDs = nullptr;
    clGetPlatformInfo_func clGetPlatformInfo = nullptr;
    clRetainDevice_func clRetainDevice = nullptr;
    clReleaseDevice_func clReleaseDevice = nullptr;
    clCreateContext_func clCreateContext = nullptr;
    clFinish_func clFinish = nullptr;
    clGetDeviceIDs_func clGetDeviceIDs = nullptr;
    clEnqueueFillBuffer_func clEnqueueFillBuffer = nullptr;
    clCreateImage_func clCreateImage = nullptr;
    clEnqueueReadImage_func clEnqueueReadImage = nullptr;
    clEnqueueWriteImage_func clEnqueueWriteImage = nullptr;
    clEnqueueMapImage_func clEnqueueMapImage = nullptr;
    clGetKernelWorkGroupInfo_func clGetKernelWorkGroupInfo = nullptr;
    clWaitForEvents_func clWaitForEvents = nullptr;
    clGetEventProfilingInfo_func clGetEventProfilingInfo = nullptr;
    clCreateImage2D_func clCreateImage2D = nullptr;
    clRetainMemObject_func clRetainMemObject = nullptr;
    clRetainEvent_func clRetainEvent = nullptr;

 private:
    CLSymbols(CL_LOADER_DYNLIB_HANDLE libraryHandle)
    {
        if (libraryHandle == nullptr) {
#if defined(_WIN32)
            std::cerr
                << "OpenCL function invoked without call to CLLoader::Init()"
                << std::endl;
#else
            std::cerr
                << "OpenCL function invoked without call to CLLoader::Init() "
                << dlerror() << std::endl;
#endif
            return;
        }

        // TODO: We can also load these lazily, but then it might hide runtime
        // issues
        //
        REINTERPRET_CAST_FUNC(clBuildProgram);
        REINTERPRET_CAST_FUNC(clEnqueueNDRangeKernel)
        REINTERPRET_CAST_FUNC(clSetKernelArg);
        REINTERPRET_CAST_FUNC(clReleaseKernel)
        REINTERPRET_CAST_FUNC(clCreateProgramWithSource);
        REINTERPRET_CAST_FUNC(clCreateBuffer);
        REINTERPRET_CAST_FUNC(clRetainKernel);
        REINTERPRET_CAST_FUNC(clCreateKernel);
        REINTERPRET_CAST_FUNC(clGetProgramInfo);
        REINTERPRET_CAST_FUNC(clFlush);
        REINTERPRET_CAST_FUNC(clReleaseProgram);
        REINTERPRET_CAST_FUNC(clRetainContext);
        REINTERPRET_CAST_FUNC(clCreateProgramWithBinary);
        REINTERPRET_CAST_FUNC(clReleaseCommandQueue);
        REINTERPRET_CAST_FUNC(clEnqueueMapBuffer);
        REINTERPRET_CAST_FUNC(clRetainProgram);
        REINTERPRET_CAST_FUNC(clGetProgramBuildInfo);
        REINTERPRET_CAST_FUNC(clEnqueueReadBuffer);
        REINTERPRET_CAST_FUNC(clEnqueueWriteBuffer);
        REINTERPRET_CAST_FUNC(clReleaseEvent);
        REINTERPRET_CAST_FUNC(clRetainCommandQueue);
        REINTERPRET_CAST_FUNC(clEnqueueUnmapMemObject);
        REINTERPRET_CAST_FUNC(clReleaseMemObject);
        REINTERPRET_CAST_FUNC(clCreateCommandQueue);
        REINTERPRET_CAST_FUNC(clCreateContextFromType);
        REINTERPRET_CAST_FUNC(clGetContextInfo);
        REINTERPRET_CAST_FUNC(clGetSupportedImageFormats);
        REINTERPRET_CAST_FUNC(clGetDeviceInfo);
        REINTERPRET_CAST_FUNC(clGetPlatformIDs);
        REINTERPRET_CAST_FUNC(clGetPlatformInfo);
        REINTERPRET_CAST_FUNC(clRetainDevice);
        REINTERPRET_CAST_FUNC(clReleaseDevice);
        REINTERPRET_CAST_FUNC(clCreateContext);
        REINTERPRET_CAST_FUNC(clFinish);
        REINTERPRET_CAST_FUNC(clGetDeviceIDs);
        REINTERPRET_CAST_FUNC(clCreateImage);
        REINTERPRET_CAST_FUNC(clEnqueueReadImage);
        REINTERPRET_CAST_FUNC(clEnqueueWriteImage);
        REINTERPRET_CAST_FUNC(clEnqueueMapImage);
        REINTERPRET_CAST_FUNC(clGetKernelWorkGroupInfo);
        REINTERPRET_CAST_FUNC(clWaitForEvents);
        REINTERPRET_CAST_FUNC(clGetEventProfilingInfo);
        REINTERPRET_CAST_FUNC(clCreateImage2D);
        REINTERPRET_CAST_FUNC(clRetainMemObject);
        REINTERPRET_CAST_FUNC(clRetainEvent);
    }
};

std::shared_ptr<CLSymbols> CLSymbols::symbolsInstance(nullptr);
}  // namespace

class Impl
{
 public:
    static Impl& Instance()
    {
        static Impl impl;
        return impl;
    }

    void Exit()
    {
        if (handle) {
            CL_LOADER_CLCLOSE(handle);
            handle = nullptr;
        }
        CLSymbols::Reset();
    }

    const std::string& GetLibPath()
    {
        return openedLib;
    }

    int Init()
    {
#if defined(_WIN32)
        const char* openclLibPath[] = {"C:\\Windows\\System32\\OpenCL.dll",
                                       "OpenCL.dll"};
        const int numOpenclLibPath = 2;
#elif defined(_WIN64)
        const char* openclLibPath[] = {"C:\\Windows\\SysWOW64\\OpenCL.dll",
                                       "OpenCL.dll"};
        const int numOpenclLibPath = 2;

#elif defined(__APPLE__)
        const char* openclLibPath[] = {
            "/System/Library/Frameworks/OpenCL.framework/OpenCL"
            "/Library/Frameworks/OpenCL.framework/OpenCL"};
        const int numOpenclLibPath = 2;

#elif defined(__ANDROID_API__)
        const char* openclLibPath[] = {
            // Typical libOpenCL location
            "/system/lib/libOpenCL.so",
            "/system/lib/egl/libOpenCL.so",
            "/system/vendor/lib/libOpenCL.so",
            "/system/vendor/lib/egl/libOpenCL.so",
            "/system/lib64/libOpenCL.so",
            "/system/lib64/egl/libOpenCL.so",
            "/system/vendor/lib64/libOpenCL.so",
            "/system/vendor/lib64/egl/libOpenCL.so",
            // Qualcomm Adreno A3xx
            "/system/lib/libllvm-a3xx.so",
            // ARM Mali series
            "/system/lib/libGLES_mali.so",
            "/system/lib/egl/libGLES_mali.so",
            "/system/vendor/lib/libGLES_mali.so",
            "/system/vendor/lib/egl/libGLES_mali.so",
            "/system/lib64/libGLES_mali.so",
            "/system/lib64/egl/libGLES_mali.so",
            "/system/vendor/lib64/libGLES_mali.so",
            "/system/vendor/lib64/egl/libGLES_mali.so",
            // Imagination PowerVR Series
            "/system/lib/libPVROCL.so",
            "/system/lib/egl/libPVROCL.so",
            "/system/vendor/lib/libPVROCL.so",
            "/system/vendor/lib/egl/libPVROCL.so",
            "/system/lib64/libPVROCL.so",
            "/system/lib64/egl/libPVROCL.so",
            "/system/vendor/lib64/libPVROCL.so",
            "/system/vendor/lib64/egl/libPVROCL.so",
            // Last try
            "libOpenCL.so",
            "libGLES_mali.so",
            "libPVROCL.so"};
        const int numOpenclLibPath = 28;

#else
        const char* openclLibPath[] = {"libOpenCL.so"};
        const int numOpenclLibPath = 1;
#endif

        for (int i = 0; i < numOpenclLibPath; i++) {
            if ((handle = CL_LOADER_DLOPEN(openclLibPath[i]))) {
                CL_LOADER_LOGD("Index %d, using the Shared library:%s\n",
                               i,
                               openclLibPath[i]);
                openedLib = openclLibPath[i];
                CL_LOADER_LOGD("Loaded OpenCL library:%s\n", openedLib.c_str());
                break;
            }
        }

        if (handle == NULL) {
            return CL_LOADER_FAILED_LOCATE_LIB_OPENCL;
        }

        // Check if all the symbols are loaded successfully.
        if (CLSymbols::Get().clBuildProgram == nullptr ||
            CLSymbols::Get().clEnqueueNDRangeKernel == nullptr ||
            CLSymbols::Get().clSetKernelArg == nullptr ||
            CLSymbols::Get().clReleaseKernel == nullptr ||
            CLSymbols::Get().clCreateProgramWithSource == nullptr ||
            CLSymbols::Get().clCreateBuffer == nullptr ||
            CLSymbols::Get().clRetainKernel == nullptr ||
            CLSymbols::Get().clCreateKernel == nullptr ||
            CLSymbols::Get().clGetProgramInfo == nullptr ||
            CLSymbols::Get().clFlush == nullptr ||
            CLSymbols::Get().clReleaseProgram == nullptr ||
            CLSymbols::Get().clRetainContext == nullptr ||
            CLSymbols::Get().clCreateProgramWithBinary == nullptr ||
            CLSymbols::Get().clReleaseCommandQueue == nullptr ||
            CLSymbols::Get().clEnqueueMapBuffer == nullptr ||
            CLSymbols::Get().clRetainProgram == nullptr ||
            CLSymbols::Get().clGetProgramBuildInfo == nullptr ||
            CLSymbols::Get().clEnqueueReadBuffer == nullptr ||
            CLSymbols::Get().clEnqueueWriteBuffer == nullptr ||
            CLSymbols::Get().clReleaseEvent == nullptr ||
            CLSymbols::Get().clRetainCommandQueue == nullptr ||
            CLSymbols::Get().clEnqueueUnmapMemObject == nullptr ||
            CLSymbols::Get().clReleaseMemObject == nullptr ||
            CLSymbols::Get().clCreateCommandQueue == nullptr ||
            CLSymbols::Get().clCreateContextFromType == nullptr ||
            CLSymbols::Get().clGetContextInfo == nullptr ||
            CLSymbols::Get().clGetDeviceInfo == nullptr ||
            CLSymbols::Get().clGetPlatformIDs == nullptr ||
            CLSymbols::Get().clGetPlatformInfo == nullptr ||
            CLSymbols::Get().clRetainDevice == nullptr ||
            CLSymbols::Get().clReleaseDevice == nullptr ||
            CLSymbols::Get().clCreateContext == nullptr ||
            CLSymbols::Get().clFinish == nullptr ||
            CLSymbols::Get().clGetDeviceIDs == nullptr ||
            CLSymbols::Get().clCreateImage == nullptr ||
            CLSymbols::Get().clEnqueueMapImage == nullptr ||
            CLSymbols::Get().clGetKernelWorkGroupInfo == nullptr ||
            CLSymbols::Get().clWaitForEvents == nullptr ||
            CLSymbols::Get().clGetEventProfilingInfo == nullptr ||
            CLSymbols::Get().clCreateImage2D == nullptr ||
            CLSymbols::Get().clRetainMemObject == nullptr ||
            CLSymbols::Get().clRetainEvent == nullptr ||
            CLSymbols::Get().clEnqueueReadImage == nullptr ||
            CLSymbols::Get().clEnqueueWriteImage == nullptr ||
            CLSymbols::Get().clGetSupportedImageFormats == nullptr) {
            return CL_LOADER_FAILED_MAP_SYMBOL;
        }

        return CL_LOADER_SUCCESS;
    }

 private:
    Impl() : openedLib("Unknown location")
    {
    }

 private:
    std::string openedLib;
};

int Loader::Init()
{
    return Impl::Instance().Init();
}

const std::string& Loader::GetLibPath()
{
    return Impl::Instance().GetLibPath();
}

void Loader::Exit()
{
    return Impl::Instance().Exit();
}

}  // namespace OpenCLHelper

using CLSymbols = OpenCLHelper::CLSymbols;

CL_LOADER_EXPORT cl_int clBuildProgram(
    cl_program program,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* options,
    void(CL_CALLBACK* pfn_notify)(cl_program program, void* user_data),
    void* user_data)
{
    auto func = CLSymbols::Get().clBuildProgram;
    if (func != nullptr) {
        return func(
            program, num_devices, device_list, options, pfn_notify, user_data);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue,
                                               cl_kernel kernel,
                                               cl_uint work_dim,
                                               const size_t* global_work_offset,
                                               const size_t* global_work_size,
                                               const size_t* local_work_size,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueNDRangeKernel;
    if (func != nullptr) {
        return func(command_queue,
                    kernel,
                    work_dim,
                    global_work_offset,
                    global_work_size,
                    local_work_size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clSetKernelArg(cl_kernel kernel,
                                       cl_uint arg_index,
                                       size_t arg_size,
                                       const void* arg_value)
{
    auto func = CLSymbols::Get().clSetKernelArg;
    if (func != nullptr) {
        return func(kernel, arg_index, arg_size, arg_value);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clReleaseMemObject(cl_mem memobj)
{
    auto func = CLSymbols::Get().clReleaseMemObject;
    if (func != nullptr) {
        return func(memobj);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueUnmapMemObject(cl_command_queue command_queue,
                                                cl_mem memobj,
                                                void* mapped_ptr,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event* event_wait_list,
                                                cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueUnmapMemObject;
    if (func != nullptr) {
        return func(command_queue,
                    memobj,
                    mapped_ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clRetainCommandQueue(cl_command_queue command_queue)
{
    auto func = CLSymbols::Get().clRetainCommandQueue;
    if (func != nullptr) {
        return func(command_queue);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clReleaseContext(cl_context context)
{
    auto func = CLSymbols::Get().clReleaseContext;
    if (func != nullptr) {
        return func(context);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}
CL_LOADER_EXPORT cl_int clReleaseEvent(cl_event event)
{
    auto func = CLSymbols::Get().clReleaseEvent;
    if (func != nullptr) {
        return func(event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueWriteBuffer(cl_command_queue command_queue,
                                             cl_mem buffer,
                                             cl_bool blocking_write,
                                             size_t offset,
                                             size_t size,
                                             const void* ptr,
                                             cl_uint num_events_in_wait_list,
                                             const cl_event* event_wait_list,
                                             cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueWriteBuffer;
    if (func != nullptr) {
        return func(command_queue,
                    buffer,
                    blocking_write,
                    offset,
                    size,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueReadBuffer(cl_command_queue command_queue,
                                            cl_mem buffer,
                                            cl_bool blocking_read,
                                            size_t offset,
                                            size_t size,
                                            void* ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueReadBuffer;
    if (func != nullptr) {
        return func(command_queue,
                    buffer,
                    blocking_read,
                    offset,
                    size,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetProgramBuildInfo(cl_program program,
                                              cl_device_id device,
                                              cl_program_build_info param_name,
                                              size_t param_value_size,
                                              void* param_value,
                                              size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetProgramBuildInfo;
    if (func != nullptr) {
        return func(program,
                    device,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clRetainProgram(cl_program program)
{
    auto func = CLSymbols::Get().clRetainProgram;
    if (func != nullptr) {
        return func(program);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

void* clEnqueueMapBuffer(cl_command_queue command_queue,
                         cl_mem buffer,
                         cl_bool blocking_map,
                         cl_map_flags map_flags,
                         size_t offset,
                         size_t size,
                         cl_uint num_events_in_wait_list,
                         const cl_event* event_wait_list,
                         cl_event* event,
                         cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clEnqueueMapBuffer;
    if (func != nullptr) {
        return func(command_queue,
                    buffer,
                    blocking_map,
                    map_flags,
                    offset,
                    size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event,
                    errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clReleaseCommandQueue(cl_command_queue command_queue)
{
    auto func = CLSymbols::Get().clReleaseCommandQueue;
    if (func != nullptr) {
        return func(command_queue);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_program
clCreateProgramWithBinary(cl_context context,
                          cl_uint num_devices,
                          const cl_device_id* device_list,
                          const size_t* lengths,
                          const unsigned char** binaries,
                          cl_int* binary_status,
                          cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateProgramWithBinary;
    if (func != nullptr) {
        return func(context,
                    num_devices,
                    device_list,
                    lengths,
                    binaries,
                    binary_status,
                    errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clRetainContext(cl_context context)
{
    auto func = CLSymbols::Get().clRetainContext;
    if (func != nullptr) {
        return func(context);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clReleaseProgram(cl_program program)
{
    auto func = CLSymbols::Get().clReleaseProgram;
    if (func != nullptr) {
        return func(program);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clFlush(cl_command_queue command_queue)
{
    auto func = CLSymbols::Get().clFlush;
    if (func != nullptr) {
        return func(command_queue);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetProgramInfo(cl_program program,
                                         cl_program_info param_name,
                                         size_t param_value_size,
                                         void* param_value,
                                         size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetProgramInfo;
    if (func != nullptr) {
        return func(program,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_kernel clCreateKernel(cl_program program,
                                          const char* kernel_name,
                                          cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateKernel;
    if (func != nullptr) {
        return func(program, kernel_name, errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clRetainKernel(cl_kernel kernel)
{
    auto func = CLSymbols::Get().clRetainKernel;
    if (func != nullptr) {
        return func(kernel);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_mem clCreateBuffer(cl_context context,
                                       cl_mem_flags flags,
                                       size_t size,
                                       void* host_ptr,
                                       cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateBuffer;
    if (func != nullptr) {
        return func(context, flags, size, host_ptr, errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_program clCreateProgramWithSource(cl_context context,
                                                      cl_uint count,
                                                      const char** strings,
                                                      const size_t* lengths,
                                                      cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateProgramWithSource;
    if (func != nullptr) {
        return func(context, count, strings, lengths, errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clReleaseKernel(cl_kernel kernel)
{
    auto func = CLSymbols::Get().clReleaseKernel;
    if (func != nullptr) {
        return func(kernel);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_command_queue
clCreateCommandQueue(cl_context context,
                     cl_device_id device,
                     cl_command_queue_properties properties,
                     cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateCommandQueue;
    if (func != nullptr) {
        return func(context, device, properties, errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_context clCreateContextFromType(
    const cl_context_properties* properties,
    cl_device_type device_type,
    void(CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data,
    cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateContextFromType;
    if (func != nullptr) {
        return func(
            properties, device_type, pfn_notify, user_data, errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clGetContextInfo(cl_context context,
                                         cl_context_info param_name,
                                         size_t param_value_size,
                                         void* param_value,
                                         size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetContextInfo;
    if (func != nullptr) {
        return func(context,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int
clGetSupportedImageFormats(cl_context context,
                           cl_mem_flags flags,
                           cl_mem_object_type image_type,
                           cl_uint num_entries,
                           cl_image_format* image_formats,
                           cl_uint* num_image_formats)
{
    auto func = CLSymbols::Get().clGetSupportedImageFormats;
    if (func != nullptr) {
        return func(context,
                    flags,
                    image_type,
                    num_entries,
                    image_formats,
                    num_image_formats);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetDeviceInfo(cl_device_id device,
                                        cl_device_info param_name,
                                        size_t param_value_size,
                                        void* param_value,
                                        size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetDeviceInfo;
    if (func != nullptr) {
        return func(device,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetPlatformIDs(cl_uint num_entries,
                                         cl_platform_id* platforms,
                                         cl_uint* num_platforms)
{
    auto func = CLSymbols::Get().clGetPlatformIDs;
    if (func != nullptr) {
        return func(num_entries, platforms, num_platforms);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetPlatformInfo(cl_platform_id platform,
                                          cl_platform_info param_name,
                                          size_t param_value_size,
                                          void* param_value,
                                          size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetPlatformInfo;
    if (func != nullptr) {
        return func(platform,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clRetainDevice(cl_device_id device)
{
    auto func = CLSymbols::Get().clRetainDevice;
    if (func != nullptr) {
        return func(device);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clReleaseDevice(cl_device_id device)
{
    auto func = CLSymbols::Get().clReleaseDevice;
    if (func != nullptr) {
        return func(device);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_context clCreateContext(
    const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void(CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data,
    cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateContext;
    if (func != nullptr) {
        return func(properties,
                    num_devices,
                    devices,
                    pfn_notify,
                    user_data,
                    errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clFinish(cl_command_queue command_queue)
{
    auto func = CLSymbols::Get().clFinish;
    if (func != nullptr) {
        return func(command_queue);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetDeviceIDs(cl_platform_id platform,
                                       cl_device_type device_type,
                                       cl_uint num_entries,
                                       cl_device_id* devices,
                                       cl_uint* num_devices)
{
    auto func = CLSymbols::Get().clGetDeviceIDs;
    if (func != nullptr) {
        return func(platform, device_type, num_entries, devices, num_devices);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueFillBuffer(cl_command_queue command_queue,
                                            cl_mem buffer,
                                            const void* pattern,
                                            size_t pattern_size,
                                            size_t offset,
                                            size_t size,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueFillBuffer;
    if (func != nullptr) {
        return func(command_queue,
                    buffer,
                    pattern,
                    pattern_size,
                    offset,
                    size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_mem clCreateImage(cl_context context,
                                      cl_mem_flags flags,
                                      const cl_image_format* image_format,
                                      const cl_image_desc* image_desc,
                                      void* host_ptr,
                                      cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateImage;
    if (func != nullptr) {
        return func(
            context, flags, image_format, image_desc, host_ptr, errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

void* clEnqueueMapImage(cl_command_queue command_queue,
                        cl_mem image,
                        cl_bool blocking_map,
                        cl_map_flags map_flags,
                        const size_t* origin,
                        const size_t* region,
                        size_t* image_row_pitch,
                        size_t* image_slice_pitch,
                        cl_uint num_events_in_wait_list,
                        const cl_event* event_wait_list,
                        cl_event* event,
                        cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clEnqueueMapImage;
    if (func != nullptr) {
        return func(command_queue,
                    image,
                    blocking_map,
                    map_flags,
                    origin,
                    region,
                    image_row_pitch,
                    image_slice_pitch,
                    num_events_in_wait_list,
                    event_wait_list,
                    event,
                    errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueReadImage(cl_command_queue command_queue,
                                           cl_mem image,
                                           cl_bool blocking_map,
                                           const size_t* origin,
                                           const size_t* region,
                                           size_t image_row_pitch,
                                           size_t image_slice_pitch,
                                           void* ptr,
                                           cl_uint num_events_in_wait_list,
                                           const cl_event* event_wait_list,
                                           cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueReadImage;
    if (func != nullptr) {
        return func(command_queue,
                    image,
                    blocking_map,
                    origin,
                    region,
                    image_row_pitch,
                    image_slice_pitch,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clEnqueueWriteImage(cl_command_queue command_queue,
                                            cl_mem image,
                                            cl_bool blocking_map,
                                            const size_t* origin,
                                            const size_t* region,
                                            size_t image_row_pitch,
                                            size_t image_slice_pitch,
                                            const void* ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event)
{
    auto func = CLSymbols::Get().clEnqueueWriteImage;
    if (func != nullptr) {
        return func(command_queue,
                    image,
                    blocking_map,
                    origin,
                    region,
                    image_row_pitch,
                    image_slice_pitch,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int
clGetKernelWorkGroupInfo(cl_kernel kernel,
                         cl_device_id device,
                         cl_kernel_work_group_info param_name,
                         size_t param_value_size,
                         void* param_value,
                         size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetKernelWorkGroupInfo;
    if (func != nullptr) {
        return func(kernel,
                    device,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clWaitForEvents(cl_uint num_events,
                                        const cl_event* event_list)
{
    auto func = CLSymbols::Get().clWaitForEvents;
    if (func != nullptr) {
        return func(num_events, event_list);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clGetEventProfilingInfo(cl_event event,
                                                cl_profiling_info param_name,
                                                size_t param_value_size,
                                                void* param_value,
                                                size_t* param_value_size_ret)
{
    auto func = CLSymbols::Get().clGetEventProfilingInfo;
    if (func != nullptr) {
        return func(event,
                    param_name,
                    param_value_size,
                    param_value,
                    param_value_size_ret);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_mem clCreateImage2D(cl_context context,
                                        cl_mem_flags flags,
                                        const cl_image_format* format,
                                        size_t width,
                                        size_t height,
                                        size_t row_pitch,
                                        void* host_ptr,
                                        cl_int* errcode_ret)
{
    auto func = CLSymbols::Get().clCreateImage2D;
    if (func != nullptr) {
        return func(context,
                    flags,
                    format,
                    width,
                    height,
                    row_pitch,
                    host_ptr,
                    errcode_ret);
    } else {
        if (errcode_ret != nullptr) {
            *errcode_ret = CL_LOADER_FAILED_MAP_SYMBOL;
        }
        return nullptr;
    }
}

CL_LOADER_EXPORT cl_int clRetainMemObject(cl_mem mem)
{
    auto func = CLSymbols::Get().clRetainMemObject;
    if (func != nullptr) {
        return func(mem);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}

CL_LOADER_EXPORT cl_int clRetainEvent(cl_event event)
{
    auto func = CLSymbols::Get().clRetainEvent;
    if (func != nullptr) {
        return func(event);
    } else {
        return CL_LOADER_FAILED_MAP_SYMBOL;
    }
}
#endif
