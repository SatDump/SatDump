#pragma once

#include "core/exception.h"
#include "logger.h"
#include "small_aaroniartsaapli.h" // #include <aaroniartsaapi.h>
#include <cstdint>

#ifdef _WIN32
#include "libs/dlfcn/dlfcn.h"
#else
#include <dlfcn.h>
#endif

extern std::string rtsa_api_path;

class RTSAApiInstance
{
public:
    uint32_t (*AARTSAAPI_Version)(void);
    AARTSAAPI_Result (*AARTSAAPI_Init)(uint32_t memory);
    AARTSAAPI_Result (*AARTSAAPI_Shutdown)(void);
    AARTSAAPI_Result (*AARTSAAPI_Open)(AARTSAAPI_Handle *handle);
    AARTSAAPI_Result (*AARTSAAPI_Close)(AARTSAAPI_Handle *handle);
    AARTSAAPI_Result (*AARTSAAPI_RescanDevices)(AARTSAAPI_Handle *handle, int timeout);
    AARTSAAPI_Result (*AARTSAAPI_EnumDevice)(AARTSAAPI_Handle *handle, const wchar_t *type, int32_t index, AARTSAAPI_DeviceInfo *dinfo);
    AARTSAAPI_Result (*AARTSAAPI_OpenDevice)(AARTSAAPI_Handle *handle, AARTSAAPI_Device *dhandle, const wchar_t *type, const wchar_t *serialNumber);
    AARTSAAPI_Result (*AARTSAAPI_CloseDevice)(AARTSAAPI_Handle *handle, AARTSAAPI_Device *dhandle);
    AARTSAAPI_Result (*AARTSAAPI_ConfigRoot)(AARTSAAPI_Device *dhandle, AARTSAAPI_Config *config);
    AARTSAAPI_Result (*AARTSAAPI_ConfigFind)(AARTSAAPI_Device *dhandle, AARTSAAPI_Config *group, AARTSAAPI_Config *config, const wchar_t *name);
    AARTSAAPI_Result (*AARTSAAPI_ConfigSetFloat)(AARTSAAPI_Device *dhandle, AARTSAAPI_Config *config, double value);
    AARTSAAPI_Result (*AARTSAAPI_ConfigSetString)(AARTSAAPI_Device *dhandle, AARTSAAPI_Config *config, const wchar_t *value);
    AARTSAAPI_Result (*AARTSAAPI_ConnectDevice)(AARTSAAPI_Device *dhandle);
    AARTSAAPI_Result (*AARTSAAPI_DisconnectDevice)(AARTSAAPI_Device *dhandle);
    AARTSAAPI_Result (*AARTSAAPI_StartDevice)(AARTSAAPI_Device *dhandle);
    AARTSAAPI_Result (*AARTSAAPI_StopDevice)(AARTSAAPI_Device *dhandle);
    AARTSAAPI_Result (*AARTSAAPI_GetPacket)(AARTSAAPI_Device *dhandle, int32_t channel, int32_t index, AARTSAAPI_Packet *packet);
    AARTSAAPI_Result (*AARTSAAPI_ConsumePackets)(AARTSAAPI_Device *dhandle, int32_t channel, int32_t num);

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
    RTSAApiInstance(std::string path)
    {
        logger->trace("Loading RTSA API from " + path + "...");

        dynlib = dlopen(path.c_str(), RTLD_LAZY);
        if (!dynlib)
            throw satdump_exception("Error loading " + path + "! Error : " + std::string(dlerror()));

        AARTSAAPI_Version = (uint32_t (*)(void))tryLoadFunc("AARTSAAPI_Version");
        AARTSAAPI_Init = (AARTSAAPI_Result(*)(uint32_t))tryLoadFunc("AARTSAAPI_Init");
        AARTSAAPI_Shutdown = (AARTSAAPI_Result(*)(void))tryLoadFunc("AARTSAAPI_Shutdown");
        AARTSAAPI_Open = (AARTSAAPI_Result(*)(AARTSAAPI_Handle *))tryLoadFunc("AARTSAAPI_Open");
        AARTSAAPI_Close = (AARTSAAPI_Result(*)(AARTSAAPI_Handle *))tryLoadFunc("AARTSAAPI_Close");
        AARTSAAPI_RescanDevices = (AARTSAAPI_Result(*)(AARTSAAPI_Handle *, int))tryLoadFunc("AARTSAAPI_RescanDevices");
        AARTSAAPI_EnumDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Handle *, const wchar_t *, int32_t, AARTSAAPI_DeviceInfo *))tryLoadFunc("AARTSAAPI_EnumDevice");
        AARTSAAPI_OpenDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Handle *, AARTSAAPI_Device *, const wchar_t *, const wchar_t *))tryLoadFunc("AARTSAAPI_OpenDevice");
        AARTSAAPI_CloseDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Handle *, AARTSAAPI_Device *))tryLoadFunc("AARTSAAPI_CloseDevice");
        AARTSAAPI_ConfigRoot = (AARTSAAPI_Result(*)(AARTSAAPI_Device *, AARTSAAPI_Config *))tryLoadFunc("AARTSAAPI_ConfigRoot");
        AARTSAAPI_ConfigFind = (AARTSAAPI_Result(*)(AARTSAAPI_Device *, AARTSAAPI_Config *, AARTSAAPI_Config *, const wchar_t *))tryLoadFunc("AARTSAAPI_ConfigFind");
        AARTSAAPI_ConfigSetFloat = (AARTSAAPI_Result(*)(AARTSAAPI_Device *, AARTSAAPI_Config *, double))tryLoadFunc("AARTSAAPI_ConfigSetFloat");
        AARTSAAPI_ConfigSetString = (AARTSAAPI_Result(*)(AARTSAAPI_Device *, AARTSAAPI_Config *, const wchar_t *))tryLoadFunc("AARTSAAPI_ConfigSetString");
        AARTSAAPI_ConnectDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Device *))tryLoadFunc("AARTSAAPI_ConnectDevice");
        AARTSAAPI_DisconnectDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Device *))tryLoadFunc("AARTSAAPI_DisconnectDevice");
        AARTSAAPI_StartDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Device *))tryLoadFunc("AARTSAAPI_StartDevice");
        AARTSAAPI_StopDevice = (AARTSAAPI_Result(*)(AARTSAAPI_Device *))tryLoadFunc("AARTSAAPI_StopDevice");
        AARTSAAPI_GetPacket = (AARTSAAPI_Result(*)(AARTSAAPI_Device *, int32_t, int32_t, AARTSAAPI_Packet *))tryLoadFunc("AARTSAAPI_GetPacket");
        AARTSAAPI_ConsumePackets = (AARTSAAPI_Result(*)(AARTSAAPI_Device *, int32_t, int32_t))tryLoadFunc("AARTSAAPI_ConsumePackets");

        AARTSAAPI_Result error = this->AARTSAAPI_Init(AARTSAAPI_MEMORY_LARGE);
        if (error != AARTSAAPI_OK)
        {
            logger->error("Could not init the RTSA API!");
            return;
        }

        uint32_t version = this->AARTSAAPI_Version();

        logger->info("Loaded RTSA API Version %d", version);
        logger->info("RTSA API is ready!");

        is_open = true;
    }

    bool isOpen() { return is_open; }

    ~RTSAApiInstance()
    {
        logger->trace("Closing RTSA API...");
        if (is_open)
            this->AARTSAAPI_Shutdown();
        dlclose(dynlib);
    }
};

extern RTSAApiInstance *rtsa_api;