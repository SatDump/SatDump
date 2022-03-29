#include "module_demod_base.h"
#include "logger.h"
#include "imgui/imgui.h"

namespace demod
{
    BaseDemodModule::BaseDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                        constellation(100.0f / 127.0f, 100.0f / 127.0f, demod_constellation_size)
    {
        // Parameters parsing
        if (parameters.count("buffer_size") > 0)
            d_buffer_size = parameters["buffer_size"].get<long>();

        if (parameters.count("samplerate") > 0)
            d_samplerate = parameters["samplerate"].get<long>();
        else
            throw std::runtime_error("Samplerate parameter must be present!");

        if (parameters.count("symbolrate") > 0)
            d_symbolrate = parameters["symbolrate"].get<long>();

        if (parameters.count("agc_rate") > 0)
            d_agc_rate = parameters["agc_rate"].get<float>();

        if (parameters.count("dc_block") > 0)
            d_dc_block = parameters["dc_block"].get<bool>();

        if (parameters.count("iq_swap") > 0)
            d_iq_swap = parameters["iq_swap"].get<bool>();

        snr = 0;
        peak_snr = 0;
    }

    void BaseDemodModule::init()
    {
        float input_sps = (float)d_samplerate / (float)d_symbolrate;                                  // Compute input SPS
        resample = input_sps > MAX_SPS || input_sps < MIN_SPS;                                        // If SPS is out of allowed range, we resample
        int range = pow(10, (std::to_string(int(d_symbolrate)).size() - 1));                          // Avoid complex resampling
        final_samplerate = resample ? (round(d_symbolrate / range) * range) * MAX_SPS : d_samplerate; // Get the final samplerate we'll be working with
        float decimation_factor = d_samplerate / final_samplerate;                                    // Decimation factor to rescale our input buffer

        if (resample)
            d_buffer_size *= round(decimation_factor);

        final_sps = final_samplerate / (float)d_symbolrate;

        logger->debug("Input SPS : {:f}", input_sps);
        logger->debug("Resample : " + std::to_string(resample));
        logger->debug("Samplerate : {:f}", final_samplerate);
        logger->debug("Dec factor : {:f}", decimation_factor);
        logger->debug("Final SPS : {:f}", final_sps);

        // Init DSP Blocks
        if (input_data_type == DATA_FILE)
            file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size, d_iq_swap);
        if (d_dc_block)
            dc_blocker = std::make_shared<dsp::CorrectIQBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream);

        // Cleanup things a bit
        std::shared_ptr<dsp::stream<complex_t>> input_data = d_dc_block ? dc_blocker->output_stream : (input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream);

        // Init resampler if required
        if (resample)
            resampler = std::make_shared<dsp::CCRationalResamplerBlock>(input_data, final_samplerate, d_samplerate);

        // AGC
        agc = std::make_shared<dsp::AGCBlock>(resample ? resampler->output_stream : input_data, d_agc_rate, 1.0f, 1.0f, 65536);
    }

    std::vector<ModuleDataType> BaseDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_DSP_STREAM};
    }

    std::vector<ModuleDataType> BaseDemodModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    BaseDemodModule::~BaseDemodModule()
    {
    }

    void BaseDemodModule::start()
    {
        // Start
        if (input_data_type == DATA_FILE)
            file_source->start();
        if (d_dc_block)
            dc_blocker->start();
        if (resample)
            resampler->start();
        agc->start();
    }

    void BaseDemodModule::stop()
    {
        // Stop
        if (input_data_type == DATA_FILE)
            file_source->stop();
        if (d_dc_block)
            dc_blocker->stop();
        if (resample)
            resampler->stop();
        agc->stop();
    }

    void BaseDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            // Show SNR information
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            if (show_freq)
            {
                ImGui::Text("Freq : ");
                ImGui::SameLine();
                ImGui::TextColored(IMCOLOR_SYNCING, "%.0f Hz", display_freq);
            }
            snr_plot.draw(snr, peak_snr);
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::vector<std::string> BaseDemodModule::getParameters()
    {
        return {"samplerate", "symbolrate", "agc_rate", "iq_swap", "buffer_size", "dc_block", "baseband_format"};
    }
}