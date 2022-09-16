#pragma once

#include "r2iq.h"
#include "config.h"
#include <algorithm>
#include <string.h>

class conv_r2iq : public r2iqControlClass
{
public:
    conv_r2iq();
    virtual ~conv_r2iq();

    float setFreqOffset(float offset);

    void Init(float gain, ringbuffer<int16_t>* buffers, ringbuffer<float>* obuffers);
    void TurnOn() override;
    void TurnOff(void) override;

protected:

    template<bool rand> void convert_float(const int16_t *input, float* output, int size)
    {
        for(int m = 0; m < size; m++)
        {
            int16_t val;
            if (rand && (input[m] & 1))
            {
                val = input[m] ^ (-2);
            }
            else
            {
                val = input[m];
            }
            output[m*2] = float(val) * (1.0f/32768.0f);
            output[m*2+1]=0;
        }
    }

private:
    ringbuffer<int16_t>* inputbuffer;    // pointer to input buffers
    ringbuffer<float>* outputbuffer;    // pointer to ouput buffers
    int bufIdx;         // index to next buffer to be processed

    float GainScale;

    void *r2iqThreadf();   // thread function

    void * r2iqThreadf_def();
    std::mutex mutexR2iqControl;
    std::thread r2iq_thread;
};

