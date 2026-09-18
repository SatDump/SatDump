#ifndef SDRPLAY_API_DEV_H
#define SDRPLAY_API_DEV_H

#include "sdrplay_api_rsp1a.h"
#include "sdrplay_api_rsp2.h"
#include "sdrplay_api_rspDuo.h"
#include "sdrplay_api_rspDx.h"

// Dev parameter enums
typedef enum
{
    sdrplay_api_ISOCH = 0,
    sdrplay_api_BULK  = 1
} sdrplay_api_TransferModeT;

// Dev parameter structs
typedef struct 
{
    double fsHz;                        // default: 2000000.0
    unsigned char syncUpdate;           // default: 0
    unsigned char reCal;                // default: 0
} sdrplay_api_FsFreqT;

typedef struct 
{
    unsigned int sampleNum;             // default: 0
    unsigned int period;                // default: 0
} sdrplay_api_SyncUpdateT;

typedef struct 
{
    unsigned char resetGainUpdate;      // default: 0
    unsigned char resetRfUpdate;        // default: 0
    unsigned char resetFsUpdate;        // default: 0
} sdrplay_api_ResetFlagsT;   

typedef struct 
{
    double ppm;                         // default: 0.0
    sdrplay_api_FsFreqT fsFreq;
    sdrplay_api_SyncUpdateT syncUpdate; 
    sdrplay_api_ResetFlagsT resetFlags; 
    sdrplay_api_TransferModeT mode;     // default: sdrplay_api_ISOCH
    unsigned int samplesPerPkt;         // default: 0 (output param)
    sdrplay_api_Rsp1aParamsT rsp1aParams;
    sdrplay_api_Rsp2ParamsT rsp2Params;
    sdrplay_api_RspDuoParamsT rspDuoParams;
    sdrplay_api_RspDxParamsT rspDxParams;
} sdrplay_api_DevParamsT;

#endif //SDRPLAY_API_DEV_H
