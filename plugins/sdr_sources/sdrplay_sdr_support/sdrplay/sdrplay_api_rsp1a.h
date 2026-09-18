#ifndef SDRPLAY_API_RSP1A_H
#define SDRPLAY_API_RSP1A_H

#define RSPIA_NUM_LNA_STATES         10
#define RSPIA_NUM_LNA_STATES_AM      7
#define RSPIA_NUM_LNA_STATES_LBAND   9

// RSP1A parameter enums

// RSP1A parameter structs
typedef struct 
{
    unsigned char rfNotchEnable;                              // default: 0
    unsigned char rfDabNotchEnable;                           // default: 0
} sdrplay_api_Rsp1aParamsT;

typedef struct 
{
    unsigned char biasTEnable;                   // default: 0
} sdrplay_api_Rsp1aTunerParamsT;

#endif //SDRPLAY_API_RSP1A_H
