#include <functional>
#include "libsddc.h"
#include "config.h"
#include "r2iq.h"
#include "conv_r2iq.h"
#include "RadioHandler.h"

#define DEBUG 0

struct sddc
{
    SDDCStatus status;
    RadioHandlerClass* handler;
    uint8_t led;
    int samplerateidx;
    double freq;

    const float *hfRFGains;
    int hfRFGainsSize;
    const float *vhfRFGains;
    int vhfRFGainsSize;

    const float *hfIFGains;
    int hfIFGainsSize;
    const float *vhfIFGains;
    int vhfIFGainsSize;

    float ifAttenuation;
    float rfAttenuation;
    float bw;

    sddc_read_async_cb_t callback;
    void *callback_context;
};

sddc_t *current_running;

static void Callback(struct sddc *s, const float* data, uint32_t len)
{
    if(s->callback)
        s->callback(len, data, s->callback_context);
}

int sddc_get_device_count()
{
    return 1;
}

int sddc_get_device_info(struct sddc_device_info **sddc_device_infos)
{
    auto ret = new sddc_device_info();
    const char *todo = "TODO";
    ret->manufacturer = todo;
    ret->product = todo;
    ret->serial_number = todo;

    *sddc_device_infos = ret;

    return 1;
}

int sddc_free_device_info(struct sddc_device_info *sddc_device_infos)
{
    delete sddc_device_infos;
    return 0;
}

sddc_t *sddc_open(int index, const char* imagefile)
{
    auto ret_val = new sddc_t();

    fx3class *fx3 = CreateUsbHandler();
    if (fx3 == nullptr)
    {
        return nullptr;
    }

    // open the firmware
    unsigned char* res_data;
    uint32_t res_size;

    FILE *fp = fopen(imagefile, "rb");
    if (fp == nullptr)
    {
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    res_size = ftell(fp);
    res_data = (unsigned char*)malloc(res_size);
    fseek(fp, 0, SEEK_SET);
    if (fread(res_data, 1, res_size, fp) != res_size)
        return nullptr;

    bool openOK = fx3->Open(res_data, res_size);
    if (!openOK)
        return nullptr;

    ret_val->handler = new RadioHandlerClass();
#if DEBUG
    fprintf(stderr,"new RadioHandlerClass()\n");
#endif
    if (ret_val->handler->Init(fx3, [ret_val](const float* data, uint32_t len){ Callback(ret_val,data,len);}, new conv_r2iq()))
    {
        ret_val->status = SDDC_STATUS_READY;
        ret_val->samplerateidx = 0;
        ret_val->handler->UpdatemodeRF(VHFMODE);
        ret_val->vhfRFGainsSize = ret_val->handler->GetRFAttSteps(&ret_val->vhfRFGains);
        ret_val->vhfIFGainsSize = ret_val->handler->GetIFGainSteps(&ret_val->vhfIFGains);
        ret_val->handler->UpdatemodeRF(HFMODE);
        ret_val->hfRFGainsSize = ret_val->handler->GetRFAttSteps(&ret_val->hfRFGains);
        ret_val->hfIFGainsSize = ret_val->handler->GetIFGainSteps(&ret_val->hfIFGains);

#if DEBUG
        fprintf(stderr,"hfRFGainsSize=%d\n",ret_val->hfRFGainsSize);
        for(int k=0;k<ret_val->hfRFGainsSize;k++)
            fprintf(stderr,"hfRFGains[%d]=%f\n",k,ret_val->hfRFGains[k]);
        fprintf(stderr,"hfIFGainsSize=%d\n",ret_val->hfIFGainsSize);
        for(int k=0;k<ret_val->hfIFGainsSize;k++)
            fprintf(stderr,"hfIFGains[%d]=%f\n",k,ret_val->hfIFGains[k]);

        fprintf(stderr,"vhfRFGainsSize=%d\n",ret_val->hfIFGainsSize);
        for(int k=0;k<ret_val->vhfRFGainsSize;k++)
            fprintf(stderr,"vhfRFGains[%d]=%f\n",k,ret_val->vhfRFGains[k]);
        fprintf(stderr,"vhfIFGainsSize=%d\n",ret_val->hfIFGainsSize);
        for(int k=0;k<ret_val->vhfIFGainsSize;k++)
            fprintf(stderr,"vhfIFGains[%d]=%f\n",k,ret_val->vhfIFGains[k]);
#endif
    }


    return ret_val;
}

void sddc_close(sddc_t *that)
{
    if (that->handler)
        delete that->handler;
    delete that;
}

enum SDDCStatus sddc_get_status(sddc_t *t)
{
    return t->status;
}

enum SDDCHWModel sddc_get_hw_model(sddc_t *t)
{
    switch(t->handler->getModel())
    {
        case RadioModel::BBRF103:
            return HW_BBRF103;
        case RadioModel::HF103:
            return HW_HF103;
        case RadioModel::RX888:
            return HW_RX888;
        case RadioModel::RX888r2:
            return HW_RX888R2;
        case RadioModel::RX888r3:
            return HW_RX888R3;
        case RadioModel::RX999:
            return HW_RX999;
        default:
            return HW_NORADIO;
    }
}

const char *sddc_get_hw_model_name(sddc_t *t)
{
    return t->handler->getName();
}

uint16_t sddc_get_firmware(sddc_t *t)
{
    return t->handler->GetFirmware();
}

const double *sddc_get_frequency_range(sddc_t *t)
{
    return nullptr;
}

enum RFMode sddc_get_rf_mode(sddc_t *t)
{
    switch(t->handler->GetmodeRF())
    {
        case HFMODE:
            return RFMode::HF_MODE;
        case VHFMODE:
            return RFMode::VHF_MODE;
        default:
            return RFMode::NO_RF_MODE;
    }
}

int sddc_set_rf_mode(sddc_t *t, enum RFMode rf_mode)
{
    switch (rf_mode)
    {
    case VHF_MODE:
        if(t->handler->GetmodeRF() == VHFMODE)
            break;
        t->handler->UpdatemodeRF(VHFMODE);
        break;
    case HF_MODE:
        if(t->handler->GetmodeRF() == HFMODE)
            break;
        t->handler->UpdatemodeRF(HFMODE);
        break;
    default:
        return -1;
    }

    return 0;
}

/* LED functions */
int sddc_led_on(sddc_t *t, uint8_t led_pattern)
{
    if (led_pattern & YELLOW_LED)
        t->handler->uptLed(0, true);
    if (led_pattern & RED_LED)
        t->handler->uptLed(1, true);
    if (led_pattern & BLUE_LED)
        t->handler->uptLed(2, true);

    t->led |= led_pattern;

    return 0;
}

int sddc_led_off(sddc_t *t, uint8_t led_pattern)
{
    if (led_pattern & YELLOW_LED)
        t->handler->uptLed(0, false);
    if (led_pattern & RED_LED)
        t->handler->uptLed(1, false);
    if (led_pattern & BLUE_LED)
        t->handler->uptLed(2, false);

    t->led &= ~led_pattern;

    return 0;
}

int sddc_led_toggle(sddc_t *t, uint8_t led_pattern)
{
    t->led = t->led ^ led_pattern;
    if (t->led & YELLOW_LED)
        t->handler->uptLed(0, false);
    if (t->led & RED_LED)
        t->handler->uptLed(1, false);
    if (t->led & BLUE_LED)
        t->handler->uptLed(2, false);

    return 0;
}


/* ADC functions */
int sddc_get_adc_dither(sddc_t *t)
{
    return t->handler->GetDither();
}

int sddc_set_adc_dither(sddc_t *t, int dither)
{
    t->handler->UptDither(dither != 0);
    return 0;
}

int sddc_get_adc_random(sddc_t *t)
{
    return t->handler->GetRand();
}

int sddc_set_adc_random(sddc_t *t, int random)
{
    t->handler->UptRand(random != 0);
    return 0;
}

/* HF block functions */
double sddc_get_hf_attenuation(sddc_t *t)
{
    return 0;
}

int sddc_set_hf_attenuation(sddc_t *t, float attenuation)
{
    return sddc_set_tuner_rf_attenuation(t,attenuation);
}

int sddc_get_hf_bias(sddc_t *t)
{
    return t->handler->GetBiasT_HF();
}

int sddc_set_hf_bias(sddc_t *t, int bias)
{
    t->handler->UpdBiasT_HF(bias != 0);
    return 0;
}


/* VHF block and VHF/UHF tuner functions */
double sddc_get_tuner_frequency(sddc_t *t)
{
    return t->freq;
}

int sddc_set_tuner_frequency(sddc_t *t, double frequency)
{
    RFMode currentMode = sddc_get_rf_mode(t);
    RFMode newMode = currentMode;
    if(frequency > 4.57e6)
    {
        //frequency -= 4.57e6;
        newMode = RFMode::VHF_MODE;
    }
    else
    {
        newMode = RFMode::HF_MODE;
    }
    if(currentMode != newMode)
    {
        sddc_set_rf_mode(t, newMode);
        sddc_set_tuner_rf_attenuation(t, t->rfAttenuation);
        sddc_set_tuner_if_attenuation(t, t->ifAttenuation);
    }
    t->freq = t->handler->TuneLO((int64_t)frequency);
    return 0;
}

int sddc_get_tuner_rf_attenuations(sddc_t *t, const float **attenuations)
{
    if(sddc_get_rf_mode(t) == RFMode::VHF_MODE)
    {
        attenuations = &t->vhfRFGains;
        return t->vhfRFGainsSize;
    }
    if(sddc_get_rf_mode(t) == RFMode::HF_MODE)
    {
        attenuations = &t->hfRFGains;
        return t->hfRFGainsSize;
    }
    return 0;
}

double sddc_get_tuner_rf_attenuation(sddc_t *t)
{
    return 0;
}

int sddc_set_tuner_rf_attenuation(sddc_t *t, float attenuation)
{
    int k=1;
    t->rfAttenuation = attenuation;
    if(sddc_get_rf_mode(t) == RFMode::VHF_MODE)
    {
        for(k=1;k<t->vhfRFGainsSize;k++)
            if(t->vhfRFGains[k]>attenuation)
                break;
        t->handler->UpdateattRF(k - 1);
    }
    if(sddc_get_rf_mode(t) == RFMode::HF_MODE)
    {
        for(k=1;k<t->hfRFGainsSize;k++)
            if(t->hfRFGains[k]>attenuation)
                break;
        t->handler->UpdateattRF(k - 1);
    }
    return k - 1;
}

int sddc_get_tuner_if_attenuations(sddc_t *t, const float **attenuations)
{
    if(sddc_get_rf_mode(t) == RFMode::VHF_MODE)
    {
        attenuations = &t->vhfIFGains;
        return t->vhfIFGainsSize;
    }
    if(sddc_get_rf_mode(t) == RFMode::HF_MODE)
    {
        attenuations = &t->hfIFGains;
        return t->hfIFGainsSize;
    }
    return 0;
}

double sddc_get_tuner_if_attenuation(sddc_t *t)
{
    return 0;
}

int sddc_set_tuner_if_attenuation(sddc_t *t, float attenuation)
{
    int k=1;
    t->ifAttenuation = attenuation;
    if(sddc_get_rf_mode(t) == RFMode::VHF_MODE)
    {
        for(k=1;k<t->vhfIFGainsSize;k++)
            if(t->vhfIFGains[k]>attenuation)
                break;
        t->handler->UpdateIFGain(k - 1);
    }
    if(sddc_get_rf_mode(t) == RFMode::HF_MODE)
    {
        for(k=1;k<t->hfIFGainsSize;k++)
            if(t->hfIFGains[k]>attenuation)
                break;
        t->handler->UpdateIFGain(k - 1);
    }
    return k - 1;
}

float sddc_get_tuner_bw(sddc_t *t)
{
    return t->bw;
}

int sddc_set_tuner_bw(sddc_t *t, float bw)
{
    if( (int)bw == 0)
        bw = 8000000.0;
    return t->handler->UpdateTunerBW(t->bw = bw);
}


int sddc_get_vhf_bias(sddc_t *t)
{
    return t->handler->GetBiasT_VHF();
}

int sddc_set_vhf_bias(sddc_t *t, int bias)
{
    t->handler->UpdBiasT_VHF(bias != 0);
    return 0;
}

double sddc_get_sample_rate(sddc_t *t)
{
    return t->handler->GetSampleRate();
    switch(t->samplerateidx)
    {
        case 0:
            return 2000000;
        case 1:
            return 4000000;
        case 2:
            return 8000000;
        case 3:
            return 16000000;
        case 4:
            return 32000000;
        case 5:
            return 64000000;
    }
    return -1;
}

int sddc_set_sample_rate(sddc_t *t, double sample_rate)
{
    t->handler->SetSampleRate(sample_rate);
    return 4;
    switch((int64_t)sample_rate)
    {
        case 32000000:
            t->samplerateidx = 4;
            break;
        case 16000000:
            t->samplerateidx = 3;
            break;
        case 8000000:
            t->samplerateidx = 2;
            break;
        case 4000000:
            t->samplerateidx = 1;
            break;
        case 2000000:
            t->samplerateidx = 0;
            break;
        case 1000000:
            t->samplerateidx = -1;
            break;
        default:
            return -1;
    }
    return 0;
}

int sddc_set_async_params(sddc_t *t, uint32_t frame_size, 
                          uint32_t num_frames, sddc_read_async_cb_t callback,
                          void *callback_context)
{
    // TODO: ignore frame_size, num_frames
    t->callback = callback;
    t->callback_context = callback_context;
    return 0;
}

int sddc_start_streaming(sddc_t *t)
{
    current_running = t;
    t->handler->Start(t->samplerateidx);
    //t->handler->Start(4);
    return 0;
}

int sddc_handle_events(sddc_t *t)
{
    return 0;
}

int sddc_stop_streaming(sddc_t *t)
{
   //  printf("STOPPING CALLED\r\n");
    t->handler->Stop();
    current_running = nullptr;
  //  printf("Called TEST\r\n");
    return 0;
}

int sddc_reset_status(sddc_t *t)
{
    return 0;
}

int sddc_read_sync(sddc_t *t, uint8_t *data, int length, int *transferred)
{
    return 0;
}
