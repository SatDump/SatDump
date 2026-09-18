#ifndef SDRPLAY_API_CALLBACK_H
#define SDRPLAY_API_CALLBACK_H

// Event callback enums
typedef enum
{
    sdrplay_api_Overload_Detected     = 0,
    sdrplay_api_Overload_Corrected    = 1,
} sdrplay_api_PowerOverloadCbEventIdT;

typedef enum
{
    sdrplay_api_MasterInitialised      = 0,
    sdrplay_api_SlaveAttached          = 1,
    sdrplay_api_SlaveDetached          = 2,
    sdrplay_api_SlaveInitialised       = 3,
    sdrplay_api_SlaveUninitialised     = 4,
    sdrplay_api_MasterDllDisappeared   = 5,
    sdrplay_api_SlaveDllDisappeared    = 6,
} sdrplay_api_RspDuoModeCbEventIdT;

typedef enum
{
    sdrplay_api_GainChange            = 0,
    sdrplay_api_PowerOverloadChange   = 1,
    sdrplay_api_DeviceRemoved         = 2,
    sdrplay_api_RspDuoModeChange      = 3,
    sdrplay_api_DeviceFailure         = 4,
} sdrplay_api_EventT;

// Event callback parameter structs
typedef struct 
{
    unsigned int gRdB;
    unsigned int lnaGRdB;
    double currGain;
} sdrplay_api_GainCbParamT;

typedef struct 
{
    sdrplay_api_PowerOverloadCbEventIdT powerOverloadChangeType;
} sdrplay_api_PowerOverloadCbParamT;

typedef struct 
{
    sdrplay_api_RspDuoModeCbEventIdT modeChangeType;
} sdrplay_api_RspDuoModeCbParamT;

// Event parameters overlay
typedef union
{
    sdrplay_api_GainCbParamT          gainParams;
    sdrplay_api_PowerOverloadCbParamT powerOverloadParams;
    sdrplay_api_RspDuoModeCbParamT    rspDuoModeParams;
} sdrplay_api_EventParamsT;

// Stream callback parameter structs
typedef struct 
{
    unsigned int firstSampleNum;
    int grChanged;
    int rfChanged;
    int fsChanged;
    unsigned int numSamples;
} sdrplay_api_StreamCbParamsT;

// Callback function prototypes
typedef void (*sdrplay_api_StreamCallback_t)(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext); 
typedef void (*sdrplay_api_EventCallback_t)(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext); 

// Callback function struct
typedef struct 
{
    sdrplay_api_StreamCallback_t StreamACbFn;
    sdrplay_api_StreamCallback_t StreamBCbFn;
    sdrplay_api_EventCallback_t  EventCbFn;
} sdrplay_api_CallbackFnsT;

#endif //SDRPLAY_API_CALLBACK_H
