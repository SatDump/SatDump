//
//  opencl_loader.h
//  libOpenCL_loader
//
//  Created by Guohui Wang on 4/23/18.
//  Copyright © 2019 Guohui Wang. All rights reserved.
//

#pragma once
#include <string>

#ifdef DEBUG
#define CL_LOADER_EXPORT
#else
#define CL_LOADER_EXPORT __attribute__((visibility("default")))
#endif

// Error message.
#define CL_LOADER_SUCCESS 0
#define CL_LOADER_FAILED_LOCATE_LIB_OPENCL -12001
#define CL_LOADER_FAILED_MAP_SYMBOL -12002
#define CL_LOADER_FAILED_REG_ATEXIT -12003

namespace OpenCLHelper {
class CL_LOADER_EXPORT Loader
{
 public:
    static int Init();
    static void Exit();
    static const std::string& GetLibPath();
};
}  // namespace OpenCLHelper
