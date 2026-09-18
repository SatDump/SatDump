#ifndef SDRPLAY_API_RX_CHANNEL_H
#define SDRPLAY_API_RX_CHANNEL_H

#include "sdrplay_api_tuner.h"
#include "sdrplay_api_control.h"
#include "sdrplay_api_rsp1a.h"
#include "sdrplay_api_rsp2.h"
#include "sdrplay_api_rspDuo.h"
#include "sdrplay_api_rspDx.h"

typedef struct 
{
    sdrplay_api_TunerParamsT        tunerParams;    
    sdrplay_api_ControlParamsT      ctrlParams;     
    sdrplay_api_Rsp1aTunerParamsT   rsp1aTunerParams; 
    sdrplay_api_Rsp2TunerParamsT    rsp2TunerParams; 
    sdrplay_api_RspDuoTunerParamsT  rspDuoTunerParams;    
    sdrplay_api_RspDxTunerParamsT   rspDxTunerParams;    
} sdrplay_api_RxChannelParamsT;

#endif //SDRPLAY_API_RX_CHANNEL_H
