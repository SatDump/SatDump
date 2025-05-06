#pragma once

#include "core/exception.h"
#include "logger.h"
#include <sdrplay_api.h>

#ifdef _WIN32
#include "libs/dlfcn/dlfcn.h"
#else
#include <dlfcn.h>
#endif

extern std::string sdrplay_api_path;

class SDRPlayApiInstance
{
public:
    int (*sdrplay_api_ApiVersion)(float *);
    int (*sdrplay_api_Close)();
    sdrplay_api_ErrT (*sdrplay_api_Open)();
    sdrplay_api_ErrT (*sdrplay_api_Update)(HANDLE, sdrplay_api_TunerSelectT, sdrplay_api_ReasonForUpdateT, sdrplay_api_ReasonForUpdateExtension1T);
    sdrplay_api_ErrT (*sdrplay_api_SelectDevice)(sdrplay_api_DeviceT *);
    sdrplay_api_ErrT (*sdrplay_api_ReleaseDevice)(sdrplay_api_DeviceT *);
    sdrplay_api_ErrT (*sdrplay_api_UnlockDeviceApi)();
    sdrplay_api_ErrT (*sdrplay_api_DebugEnable)(HANDLE, sdrplay_api_DbgLvl_t);
    sdrplay_api_ErrT (*sdrplay_api_GetDeviceParams)(HANDLE, sdrplay_api_DeviceParamsT **);
    sdrplay_api_ErrT (*sdrplay_api_Init)(HANDLE, sdrplay_api_CallbackFnsT *, void *);
    sdrplay_api_ErrT (*sdrplay_api_Uninit)(HANDLE);
    sdrplay_api_ErrT (*sdrplay_api_GetDevices)(sdrplay_api_DeviceT *, unsigned int *, unsigned int);

private:
    void *tryLoadFunc(const char *symbol)
    {
        void *ptr = dlsym(dynlib, symbol);
        const char *dlsym_error = dlerror();
        if (dlsym_error != NULL)
            logger->warn("Possible error loading symbols from plugin!");
        return ptr;
    }

    bool is_open = false;
    void *dynlib;

public:
    SDRPlayApiInstance(std::string path)
    {
        logger->trace("Loading SDRPlay API from " + path + "...");

        dynlib = dlopen(path.c_str(), RTLD_LAZY);
        if (!dynlib)
            throw satdump_exception("Error loading " + path + "! Error : " + std::string(dlerror()));

        sdrplay_api_ApiVersion = (int (*)(float *))tryLoadFunc("sdrplay_api_ApiVersion");
        sdrplay_api_Open = (sdrplay_api_ErrT(*)())tryLoadFunc("sdrplay_api_Open");
        sdrplay_api_Close = (int (*)())tryLoadFunc("sdrplay_api_Close");
        sdrplay_api_Update = (sdrplay_api_ErrT(*)(HANDLE, sdrplay_api_TunerSelectT, sdrplay_api_ReasonForUpdateT, sdrplay_api_ReasonForUpdateExtension1T))tryLoadFunc("sdrplay_api_Update");
        sdrplay_api_SelectDevice = (sdrplay_api_ErrT(*)(sdrplay_api_DeviceT *))tryLoadFunc("sdrplay_api_SelectDevice");
        sdrplay_api_ReleaseDevice = (sdrplay_api_ErrT(*)(sdrplay_api_DeviceT *))tryLoadFunc("sdrplay_api_ReleaseDevice");
        sdrplay_api_UnlockDeviceApi = (sdrplay_api_ErrT(*)())tryLoadFunc("sdrplay_api_UnlockDeviceApi");
        sdrplay_api_DebugEnable = (sdrplay_api_ErrT(*)(HANDLE, sdrplay_api_DbgLvl_t))tryLoadFunc("sdrplay_api_DebugEnable");
        sdrplay_api_GetDeviceParams = (sdrplay_api_ErrT(*)(HANDLE, sdrplay_api_DeviceParamsT **))tryLoadFunc("sdrplay_api_GetDeviceParams");
        sdrplay_api_Init = (sdrplay_api_ErrT(*)(HANDLE, sdrplay_api_CallbackFnsT *, void *))tryLoadFunc("sdrplay_api_Init");
        sdrplay_api_Uninit = (sdrplay_api_ErrT(*)(HANDLE))tryLoadFunc("sdrplay_api_Uninit");
        sdrplay_api_GetDevices = (sdrplay_api_ErrT(*)(sdrplay_api_DeviceT *, unsigned int *, unsigned int))tryLoadFunc("sdrplay_api_GetDevices");

        sdrplay_api_ErrT error = this->sdrplay_api_Open();
        if (error != sdrplay_api_Success)
        {
            logger->error("Could not open the SDRPlay API, perhaps the service is not running?");
            return;
        }

        float sdrplay_version = 0;
        if (this->sdrplay_api_ApiVersion(&sdrplay_version) != sdrplay_api_Success)
        {
            logger->error("Could not get SDRPlay API Version!");
            return;
        }

        logger->info("Loaded SDRPlay API Version %f (Compiled with %f)", sdrplay_version, (float)SDRPLAY_API_VERSION);
        logger->info("SDRPlay API is ready!");

        is_open = true;
    }

    ~SDRPlayApiInstance()
    {
        logger->trace("Closing SDRPlay API...");
        if (is_open)
            this->sdrplay_api_Close();
        dlclose(dynlib);
    }
};