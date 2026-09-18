#ifndef SDRPLAY_API_RSP2_H
#define SDRPLAY_API_RSP2_H

#define RSPII_NUM_LNA_STATES         9
#define RSPII_NUM_LNA_STATES_AMPORT  5
#define RSPII_NUM_LNA_STATES_420MHZ  6

// RSP2 parameter enums
typedef enum
{
    sdrplay_api_Rsp2_ANTENNA_A = 5,
    sdrplay_api_Rsp2_ANTENNA_B = 6,
} sdrplay_api_Rsp2_AntennaSelectT;

typedef enum
{
    sdrplay_api_Rsp2_AMPORT_1 = 1,
    sdrplay_api_Rsp2_AMPORT_2 = 0,
} sdrplay_api_Rsp2_AmPortSelectT;

// RSP2 parameter structs
typedef struct 
{
    unsigned char extRefOutputEn;                // default: 0
} sdrplay_api_Rsp2ParamsT;

typedef struct 
{
    unsigned char biasTEnable;                   // default: 0
    sdrplay_api_Rsp2_AmPortSelectT amPortSel;    // default: sdrplay_api_Rsp2_AMPORT_2
    sdrplay_api_Rsp2_AntennaSelectT antennaSel;  // default: sdrplay_api_Rsp2_ANTENNA_A
    unsigned char rfNotchEnable;                 // default: 0
} sdrplay_api_Rsp2TunerParamsT;

#endif //SDRPLAY_API_RSP2_H
