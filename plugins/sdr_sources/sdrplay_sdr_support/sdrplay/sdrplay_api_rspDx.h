#ifndef SDRPLAY_API_RSPDX_H
#define SDRPLAY_API_RSPDX_H

#include "sdrplay_api_tuner.h"

#define RSPDX_NUM_LNA_STATES               28   // Number of LNA states in all bands (except where defined differently below)
#define RSPDX_NUM_LNA_STATES_AMPORT2_0_12  19   // Number of LNA states when using AM Port 2 between 0 and 12MHz
#define RSPDX_NUM_LNA_STATES_AMPORT2_12_50 20   // Number of LNA states when using AM Port 2 between 12 and 50MHz
#define RSPDX_NUM_LNA_STATES_AMPORT2_50_60 25   // Number of LNA states when using AM Port 2 between 50 and 60MHz
#define RSPDX_NUM_LNA_STATES_VHF_BAND3     27   // Number of LNA states in VHF and Band3
#define RSPDX_NUM_LNA_STATES_420MHZ        21   // Number of LNA states in 420MHz band
#define RSPDX_NUM_LNA_STATES_LBAND         19   // Number of LNA states in L-band
#define RSPDX_NUM_LNA_STATES_DX            22   // Number of LNA states in DX path

// RSPdx parameter enums
typedef enum
{
    sdrplay_api_RspDx_ANTENNA_A = 0,
    sdrplay_api_RspDx_ANTENNA_B = 1,
    sdrplay_api_RspDx_ANTENNA_C = 2,
} sdrplay_api_RspDx_AntennaSelectT;

typedef enum
{
    sdrplay_api_RspDx_HDRMODE_BW_0_200  = 0,
    sdrplay_api_RspDx_HDRMODE_BW_0_500  = 1,
    sdrplay_api_RspDx_HDRMODE_BW_1_200  = 2,
    sdrplay_api_RspDx_HDRMODE_BW_1_700  = 3,
} sdrplay_api_RspDx_HdrModeBwT;

// RSPdx parameter structs
typedef struct 
{
    unsigned char hdrEnable;                            // default: 0
    unsigned char biasTEnable;                          // default: 0
    sdrplay_api_RspDx_AntennaSelectT antennaSel;        // default: sdrplay_api_RspDx_ANTENNA_A
    unsigned char rfNotchEnable;                        // default: 0
    unsigned char rfDabNotchEnable;                     // default: 0
} sdrplay_api_RspDxParamsT;

typedef struct 
{
    sdrplay_api_RspDx_HdrModeBwT hdrBw;                 // default: sdrplay_api_RspDx_HDRMODE_BW_1_700
} sdrplay_api_RspDxTunerParamsT;

#endif //SDRPLAY_API_RSPDX_H
