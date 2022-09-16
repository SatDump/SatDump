#pragma once

#ifdef __ANDROID__
#include <vector>

struct DevVIDPID
{
    uint16_t vid;
    uint16_t pid;
};

int getDeviceFD(int &vid, int &pid, const std::vector<DevVIDPID> allowedVidPids, std::string &path);
#endif