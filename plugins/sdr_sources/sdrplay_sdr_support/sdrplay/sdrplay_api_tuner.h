#ifndef SDRPLAY_API_TUNER_H
#define SDRPLAY_API_TUNER_H

#define MAX_BB_GR                         (59)

// Tuner parameter enums
typedef enum
{
    sdrplay_api_BW_Undefined = 0,
    sdrplay_api_BW_0_200     = 200,
    sdrplay_api_BW_0_300     = 300,
    sdrplay_api_BW_0_600     = 600,
    sdrplay_api_BW_1_536     = 1536,
    sdrplay_api_BW_5_000     = 5000,
    sdrplay_api_BW_6_000     = 6000,
    sdrplay_api_BW_7_000     = 7000,
    sdrplay_api_BW_8_000     = 8000
} sdrplay_api_Bw_MHzT;

typedef enum
{
    sdrplay_api_IF_Undefined = -1,
    sdrplay_api_IF_Zero      = 0,
    sdrplay_api_IF_0_450     = 450,
    sdrplay_api_IF_1_620     = 1620,
    sdrplay_api_IF_2_048     = 2048
} sdrplay_api_If_kHzT;

typedef enum
{
    sdrplay_api_LO_Undefined = 0,
    sdrplay_api_LO_Auto      = 1,
    sdrplay_api_LO_120MHz    = 2,
    sdrplay_api_LO_144MHz    = 3,
    sdrplay_api_LO_168MHz    = 4
} sdrplay_api_LoModeT;

typedef enum
{
    sdrplay_api_EXTENDED_MIN_GR = 0,
    sdrplay_api_NORMAL_MIN_GR   = 20
} sdrplay_api_MinGainReductionT;

typedef enum
{
    sdrplay_api_Tuner_Neither  = 0,
    sdrplay_api_Tuner_A        = 1,
    sdrplay_api_Tuner_B        = 2,
    sdrplay_api_Tuner_Both     = 3,
} sdrplay_api_TunerSelectT;

// Tuner parameter structs
typedef struct
{
    float curr;
    float max;
    float min;
} sdrplay_api_GainValuesT;

typedef struct 
{
    int gRdB;                            // default: 50
    unsigned char LNAstate;              // default: 0
    unsigned char syncUpdate;            // default: 0
    sdrplay_api_MinGainReductionT minGr; // default: sdrplay_api_NORMAL_MIN_GR
    sdrplay_api_GainValuesT gainVals;    // output parameter
} sdrplay_api_GainT;

typedef struct 
{
    double rfHz;                         // default: 200000000.0
    unsigned char syncUpdate;            // default: 0
} sdrplay_api_RfFreqT;

typedef struct 
{
    unsigned char dcCal;                 // default: 3 (Periodic mode)
    unsigned char speedUp;               // default: 0 (No speedup)
    int trackTime;                       // default: 1    (=> time in uSec = (72 * 3 * trackTime) / 24e6       = 9uSec)
    int refreshRateTime;                 // default: 2048 (=> time in uSec = (72 * 3 * refreshRateTime) / 24e6 = 18432uSec)
} sdrplay_api_DcOffsetTunerT;

typedef struct 
{
    sdrplay_api_Bw_MHzT bwType;          // default: sdrplay_api_BW_0_200
    sdrplay_api_If_kHzT ifType;          // default: sdrplay_api_IF_Zero
    sdrplay_api_LoModeT loMode;          // default: sdrplay_api_LO_Auto
    sdrplay_api_GainT gain;
    sdrplay_api_RfFreqT rfFreq;
    sdrplay_api_DcOffsetTunerT dcOffsetTuner;
} sdrplay_api_TunerParamsT;

#endif //SDRPLAY_API_TUNER_H
