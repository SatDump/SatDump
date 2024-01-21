#include "autotrack.h"
#include "logger.h"

void AutoTrackApp::start_device()
{
    if (is_started)
        return;

    set_frequency(frequency_hz);

    try
    {
        current_samplerate = source_ptr->get_samplerate();
        if (current_samplerate == 0)
            throw std::runtime_error("Samplerate not set!");

        source_ptr->start();

        // if (current_decimation > 1)
        // {
        //     decim_ptr = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(source_ptr->output_stream, 1, current_decimation);
        //     decim_ptr->start();
        //     logger->info("Setting up resampler...");
        //}

        fft->set_fft_settings(fft_size, get_samplerate(), fft_rate);
        // waterfall_plot->set_rate(fft_rate, waterfall_rate);
        fft_plot->bandwidth = get_samplerate();

        splitter->input_stream = /*current_decimation > 1 ? decim_ptr->output_stream :*/ source_ptr->output_stream;
        splitter->start();
        is_started = true;
    }
    catch (std::runtime_error &e)
    {
        logger->error(e.what());
    }
}

void AutoTrackApp::stop_device()
{
    if (!is_started)
        return;

    splitter->stop_tmp();
    // if (current_decimation > 1)
    //     decim_ptr->stop();
    source_ptr->stop();
    is_started = false;
}