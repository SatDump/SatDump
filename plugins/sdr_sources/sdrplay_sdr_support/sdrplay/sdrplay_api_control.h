#ifndef SDRPLAY_API_CONTROL_H
#define SDRPLAY_API_CONTROL_H

// Control parameter enums
typedef enum
{
    sdrplay_api_AGC_DISABLE  = 0,
    sdrplay_api_AGC_100HZ    = 1,
    sdrplay_api_AGC_50HZ     = 2,
    sdrplay_api_AGC_5HZ      = 3,
    sdrplay_api_AGC_CTRL_EN  = 4
} sdrplay_api_AgcControlT;

typedef enum
{
    sdrplay_api_ADSB_DECIMATION                  = 0,
    sdrplay_api_ADSB_NO_DECIMATION_LOWPASS       = 1,
    sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ = 2,
    sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ = 3
} sdrplay_api_AdsbModeT;

// Control parameter structs
typedef struct 
{
    unsigned char DCenable;          // default: 1
    unsigned char IQenable;          // default: 1
} sdrplay_api_DcOffsetT;

typedef struct 
{
    unsigned char enable;            // default: 0
    unsigned char decimationFactor;  // default: 1
    unsigned char wideBandSignal;    // default: 0
} sdrplay_api_DecimationT;

typedef struct 
{
    sdrplay_api_AgcControlT enable;    // default: sdrplay_api_AGC_50HZ
    int setPoint_dBfs;                 // default: -60
    unsigned short attack_ms;          // default: 0
    unsigned short decay_ms;           // default: 0
    unsigned short decay_delay_ms;     // default: 0
    unsigned short decay_threshold_dB; // default: 0
    int syncUpdate;                    // default: 0
} sdrplay_api_AgcT;

typedef struct 
{
    sdrplay_api_DcOffsetT dcOffset;
    sdrplay_api_DecimationT decimation;
    sdrplay_api_AgcT agc;
    sdrplay_api_AdsbModeT adsbMode;  //default: sdrplay_api_ADSB_DECIMATION
} sdrplay_api_ControlParamsT;

#endif //SDRPLAY_API_CONTROL_H
