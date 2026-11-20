#pragma once

#include "image/image.h"
#include "projection/thinplatespline.h"
#include <vector>

// CpuOnlyWarpGuard needs satdump::opencl::platform_id, a data symbol that is not
// declared in any header and so is not exported from the Windows DLL: it, and
// every test using it, is left out of MSVC builds.
#ifndef _MSC_VER
#ifdef USE_OPENCL
namespace satdump
{
    namespace opencl
    {
        // Defined in core/opencl.cpp but not exposed in its header. It is zero-initialised,
        // which useCL() treats as "use OpenCL platform 0" when initOpenCL() was never called.
        extern int platform_id;
    } // namespace opencl
} // namespace satdump
#endif
#endif

namespace warp_test
{
#ifndef _MSC_VER
    // Forces ImageWarper::warp() onto its CPU path for the lifetime of the guard
    struct CpuOnlyWarpGuard
    {
#ifdef USE_OPENCL
        int saved;
        CpuOnlyWarpGuard() : saved(satdump::opencl::platform_id) { satdump::opencl::platform_id = -1; }
        ~CpuOnlyWarpGuard() { satdump::opencl::platform_id = saved; }
#endif
        CpuOnlyWarpGuard(const CpuOnlyWarpGuard &) = delete;
        CpuOnlyWarpGuard &operator=(const CpuOnlyWarpGuard &) = delete;
#ifndef USE_OPENCL
        CpuOnlyWarpGuard() {}
#endif
    };
#endif

    // Distinct 16-bit gradient: value = 1000 * x + 100 * y + 7 * c
    inline satdump::image::Image gradientImage(int w, int h, int channels)
    {
        satdump::image::Image img(16, w, h, channels);
        for (int c = 0; c < channels; c++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    img.set(c, x, y, 1000 * x + 100 * y + 7 * c);
        return img;
    }

    // GCPs mapping input pixel (x, y) to lon = lon0 + res * x, lat = lat0 - res * y, sampled
    // every `step` pixels plus the last row and column (like compute_gcps)
    inline std::vector<satdump::projection::GCP> affineGcps(int w, int h, double lon0, double lat0, double res, int step = 1)
    {
        std::vector<int> xs, ys;
        for (int x = 0; x < w - 1; x += step)
            xs.push_back(x);
        xs.push_back(w - 1);
        for (int y = 0; y < h - 1; y += step)
            ys.push_back(y);
        ys.push_back(h - 1);

        std::vector<satdump::projection::GCP> g;
        for (int y : ys)
            for (int x : xs)
                g.push_back({(double)x, (double)y, lon0 + res * x, lat0 - res * y});
        return g;
    }
} // namespace warp_test
