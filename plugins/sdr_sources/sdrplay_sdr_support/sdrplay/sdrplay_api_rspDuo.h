#ifndef SDRPLAY_API_RSPduo_H
#define SDRPLAY_API_RSPduo_H

#define RSPDUO_NUM_LNA_STATES         10
#define RSPDUO_NUM_LNA_STATES_AMPORT  5
#define RSPDUO_NUM_LNA_STATES_AM      7
#define RSPDUO_NUM_LNA_STATES_LBAND   9

// RSPduo parameter enums
typedef enum
{
    sdrplay_api_RspDuoMode_Unknown      = 0,
    sdrplay_api_RspDuoMode_Single_Tuner = 1,
    sdrplay_api_RspDuoMode_Dual_Tuner   = 2,
    sdrplay_api_RspDuoMode_Master       = 4,
    sdrplay_api_RspDuoMode_Slave        = 8,
} sdrplay_api_RspDuoModeT;

typedef enum
{
    sdrplay_api_RspDuo_AMPORT_1 = 1,
    sdrplay_api_RspDuo_AMPORT_2 = 0,
} sdrplay_api_RspDuo_AmPortSelectT;

// RSPduo parameter structs
typedef struct 
{
    int extRefOutputEn;                             // default: 0
} sdrplay_api_RspDuoParamsT;

typedef struct 
{
   unsigned char resetGainUpdate;      // default: 0
   unsigned char resetRfUpdate;        // default: 0
} sdrplay_api_RspDuo_ResetSlaveFlagsT;

typedef struct
{
    unsigned char biasTEnable;                      // default: 0
    sdrplay_api_RspDuo_AmPortSelectT tuner1AmPortSel; // default: sdrplay_api_RspDuo_AMPORT_2
    unsigned char tuner1AmNotchEnable;              // default: 0
    unsigned char rfNotchEnable;                    // default: 0
    unsigned char rfDabNotchEnable;                 // default: 0
    sdrplay_api_RspDuo_ResetSlaveFlagsT resetSlaveFlags;
} sdrplay_api_RspDuoTunerParamsT;

#endif //SDRPLAY_API_RSPduo_H
