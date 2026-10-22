#pragma once
#ifdef __ANDROID__
#include <string>
#include <vector>
#include <functional>
#include "rtl-sdr.h"

extern "C" int rtlsdr_open_path(rtlsdr_dev_t **dev, const char *path);

namespace rtl_android
{
    struct Device { std::string path, name; int index; };
    std::vector<Device> devices();
    // Returns false while the Android permission dialog is pending; throws on failure.
    bool permission(const std::string &path, bool request);
    // Only for callers already on a worker (the NDSP flowgraph).
    void wait_permission(const std::string &path, const std::function<bool()> &cancelled);
}
#endif
