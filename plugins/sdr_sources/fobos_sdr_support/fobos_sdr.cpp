#include "fobos_sdr.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include <cstdint>
#include <fobos.h>

void FobosSource::set_gains()
{
    if (!is_started)
        return;

    fobos_rx_set_lna_gain(fobos_dev_ovj, lna_gain);
    fobos_rx_set_vga_gain(fobos_dev_ovj, vga_gain);
    logger->debug("Set Fobos LNA Gain to %d", lna_gain);
    logger->debug("Set Airspy Mixer to %d", vga_gain);
}

void FobosSource::open_sdr()
{
    char serials[1024];
    memset(serials, 0, sizeof(serials));
    int c = fobos_rx_list_devices(serials);

    if (c > 0)
    {
        auto split_serials(splitString(std::string((const char *)serials), ' '));

        for (int i = 0; i < split_serials.size(); i++)
        {
            if (split_serials[i] == d_sdr_id)
            {
                if (fobos_rx_open(&fobos_dev_ovj, i))
                    throw satdump_exception("Could not open Fobos device!");
                return;
            }
        }
    }
    else
        throw satdump_exception("Could not open Fobos device!");
}

void FobosSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    lna_gain = getValueOrDefault(d_settings["lna_gain"], lna_gain);
    vga_gain = getValueOrDefault(d_settings["vga_gain"], vga_gain);

    if (is_started)
    {
        set_gains();
    }
}

nlohmann::json FobosSource::get_settings()
{
    d_settings["lna_gain"] = lna_gain;
    d_settings["vga_gain"] = vga_gain;

    return d_settings;
}

void FobosSource::open()
{
    open_sdr();
    is_open = true;

    // Get available samplerates
    double dev_samplerates[128];
    uint32_t samprate_cnt;
    std::vector<double> available_samplerates = {2.5e6, 5e6};
    fobos_rx_get_samplerates(fobos_dev_ovj, dev_samplerates, &samprate_cnt);
    for (int i = samprate_cnt - 1; i >= 0; i--)
    {
        logger->trace("Fobos device has samplerate %d SPS", (int)dev_samplerates[i]);
        available_samplerates.push_back(dev_samplerates[i]);
    }

    samplerate_widget.set_list(available_samplerates, false);

    fobos_rx_close(fobos_dev_ovj);
}

void FobosSource::start()
{
    DSPSampleSource::start();
    open_sdr();

    uint64_t current_samplerate = samplerate_widget.get_value();

    // We cannot actually change the samplerate here... No LPF under 50M
    double actual_sr;
    fobos_rx_set_samplerate(fobos_dev_ovj, (current_samplerate >= 50e6) ? current_samplerate : 50e6, &actual_sr);
    logger->debug("Set Fobos samplerate to " + std::to_string(current_samplerate) + " (%f)", actual_sr);

    fobos_rx_set_direct_sampling(fobos_dev_ovj, direct_sampling);
    fobos_rx_set_clk_source(fobos_dev_ovj, 0);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();

    size_t bufsize = /*current_samplerate*/ 50e6 / 400;
    fobos_rx_start_sync(fobos_dev_ovj, bufsize);

    if (current_samplerate < 50e6)
    {
        output_stream_local = output_stream;
        decimator = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(output_stream, current_samplerate, 50e6);
        output_stream = decimator->output_stream;
        decimator->start();
    }
    else
    {
        decimator.reset();
    }

    thread_should_run = true;
    work_thread = std::thread(&FobosSource::mainThread, this);
}

void FobosSource::stop()
{
    if (decimator)
        decimator->stop();

    thread_should_run = false;
    logger->info("Waiting for the thread...");
    if (is_started)
        output_stream->stopWriter();
    if (work_thread.joinable())
        work_thread.join();
    logger->info("Thread stopped");
    if (is_started)
    {
        fobos_rx_stop_sync(fobos_dev_ovj);
        fobos_rx_close(fobos_dev_ovj);
    }
    is_started = false;
}

void FobosSource::close() {}

void FobosSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        double real_freq;
        fobos_rx_set_frequency(fobos_dev_ovj, frequency, &real_freq);
        logger->debug("Set Fobos frequency to %d (%f)", frequency, real_freq);
    }
    DSPSampleSource::set_frequency(frequency);
}

void FobosSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    ImGui::Checkbox("Direct Sampling", &direct_sampling);

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    bool gain_changed = false;

    gain_changed |= RImGui::SteppedSliderInt("LNA Gain", &lna_gain, 0, 2);
    gain_changed |= RImGui::SteppedSliderInt("VGA Gain", &vga_gain, 0, 15);

    if (gain_changed)
        set_gains();
}

void FobosSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 10e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t FobosSource::get_samplerate() { return samplerate_widget.get_value(); }

std::vector<dsp::SourceDescriptor> FobosSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    char serials[1024];
    memset(serials, 0, sizeof(serials));
    int c = fobos_rx_list_devices(serials);

    if (c > 0)
    {
        auto split_serials(splitString(std::string((const char *)serials), ' '));

        for (auto s : split_serials)
            results.push_back({"fobos", "FobosSDR " + s, s});
    }

    return results;
}
