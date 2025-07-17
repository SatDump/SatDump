#pragma once

#include <cstdint>

// Simplified <aaroniartsaapi.h> to be able
// to compile WITHOUT actually installing the API

extern "C"
{
    typedef uint32_t AARTSAAPI_Result;

#define AARTSAAPI_OK AARTSAAPI_Result(0x00000000)
#define AARTSAAPI_EMPTY AARTSAAPI_Result(0x00000001)
#define AARTSAAPI_RETRY AARTSAAPI_Result(0x00000002)

#define AARTSAAPI_MEMORY_SMALL 0
#define AARTSAAPI_MEMORY_MEDIUM 1
#define AARTSAAPI_MEMORY_LARGE 2
#define AARTSAAPI_MEMORY_LUDICROUS 3

    struct AARTSAAPI_Handle
    {
        void *d;
    };

    struct AARTSAAPI_Device
    {
        void *d;
    };

    struct AARTSAAPI_Config
    {
        void *d;
    };

    struct AARTSAAPI_Packet
    {
        int64_t cbsize;

        uint64_t streamID;
        uint64_t flags;

        double startTime;
        double endTime;
        double startFrequency;
        double stepFrequency;
        double spanFrequency;
        double rbwFrequency;

        int64_t num;
        int64_t total;
        int64_t size;
        int64_t stride;
        float *fp32;

        int64_t interleave;
    };

    struct AARTSAAPI_DeviceInfo
    {
        int64_t cbsize;

        wchar_t serialNumber[120];
        bool ready;
        bool boost;
        bool superspeed;
        bool active;
    };
}