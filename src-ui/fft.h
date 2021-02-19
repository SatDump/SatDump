#pragma once
#include <memory>
#include "pipe.h"
#include <libairspy/airspy.h>
#include <fftw3.h>
#include <dsp/pipe.h>
#include <complex>
#include <thread>

class SDRSource
{
private:
    struct airspy_device *dev;
    int gain = 10;
    float scale = 1.0;
    bool bias = false;
    float fft_buffer[2048];

    int d_samplerate;

    std::shared_ptr<satdump::Pipe> d_output_pipe;
    std::thread fft_thread;
    void fftFun();

public:
    SDRSource(int samplerate, std::shared_ptr<satdump::Pipe> output_pipe);
    void drawUI();
    void startSDR();
};