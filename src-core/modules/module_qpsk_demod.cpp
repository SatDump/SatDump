#include "module_qpsk_demod.h"
#include "common/dsp/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::size_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

QPSKDemodModule::QPSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                    d_agc_rate(parameters["agc_rate"].get<float>()),
                                                                                                                    d_samplerate(parameters["samplerate"].get<long>()),
                                                                                                                    d_symbolrate(parameters["symbolrate"].get<long>()),
                                                                                                                    d_rrc_alpha(parameters["rrc_alpha"].get<float>()),
                                                                                                                    d_rrc_taps(parameters["rrc_taps"].get<int>()),
                                                                                                                    d_loop_bw(parameters["costas_bw"].get<float>()),
                                                                                                                    d_buffer_size(parameters["buffer_size"].get<long>()),
                                                                                                                    d_dc_block(parameters.count("dc_block") > 0 ? parameters["dc_block"].get<bool>() : 0),
                                                                                                                    d_iq_swap(parameters.count("iq_swap") > 0 ? parameters["iq_swap"].get<bool>() : 0),
                                                                                                                    d_clock_gain_omega(parameters.count("clock_gain_omega") > 0 ? parameters["clock_gain_omega"].get<float>() : (pow(8.7e-3, 2) / 4.0)),
                                                                                                                    d_clock_mu(parameters.count("clock_mu") > 0 ? parameters["clock_mu"].get<float>() : 0.5f),
                                                                                                                    d_clock_gain_mu(parameters.count("clock_gain_mu") > 0 ? parameters["clock_gain_mu"].get<float>() : 8.7e-3),
                                                                                                                    d_clock_omega_relative_limit(parameters.count("clock_omega_relative_limit") > 0 ? parameters["clock_omega_relative_limit"].get<float>() : 0.005f),
                                                                                                                    constellation(100.0f / 127.0f, 100.0f / 127.0f, demod_constellation_size)
{
    // Buffers
    sym_buffer = new int8_t[d_buffer_size * 2];
    snr = 0;
    peak_snr = 0;
}

void QPSKDemodModule::init()
{
    float input_sps = (float)d_samplerate / (float)d_symbolrate;                                  // Compute input SPS
    resample = input_sps > MAX_SPS;                                                               // If SPS is over MAX_SPS, we resample
    int range = pow(10, (std::to_string(int(d_symbolrate)).size() - 1));                          // Avoid complex resampling
    float samplerate = resample ? (round(d_symbolrate / range) * range) * MAX_SPS : d_samplerate; // Get the final samplerate we'll be working with
    float decimation_factor = d_samplerate / samplerate;                                          // Decimation factor to rescale our input buffer

    if (resample)
        d_buffer_size *= round(decimation_factor);

    float sps = samplerate / (float)d_symbolrate;

    logger->debug("Input SPS : " + std::to_string(input_sps));
    logger->debug("Resample : " + std::to_string(resample));
    logger->debug("Samplerate : " + std::to_string(samplerate));
    logger->debug("Dec factor : " + std::to_string(decimation_factor));
    logger->debug("Final SPS : " + std::to_string(sps));

    // Init DSP Blocks
    if (input_data_type == DATA_FILE)
        file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);
    if (d_dc_block)
        dcb = std::make_shared<dsp::DCBlockerBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream, 1024, true);

    // Cleanup things a bit
    std::shared_ptr<dsp::stream<complex_t>> input_data = d_dc_block ? dcb->output_stream : (input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream);

    // Init resampler if required
    if (resample)
        res = std::make_shared<dsp::CCRationalResamplerBlock>(input_data, samplerate, d_samplerate);

    // AGC
    agc = std::make_shared<dsp::AGCBlock>(resample ? res->output_stream : input_data, d_agc_rate, 1.0f, 1.0f, 65536);

    // RRC
    rrc = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, dsp::firdes::root_raised_cosine(1, samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

    // Costas
    pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 4);

    // Clock recovery
    rec = std::make_shared<dsp::CCMMClockRecoveryBlock>(pll->output_stream, sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);
    //rec = std::make_shared<dsp::CCMMClockRecoveryBlock>(pll->output_stream, sps, pow(8.7e-3, 2) / 4.0, 0.5f, 8.7e-3, 0.005f);
}

std::vector<ModuleDataType> QPSKDemodModule::getInputTypes()
{
    return {DATA_FILE, DATA_DSP_STREAM};
}

std::vector<ModuleDataType> QPSKDemodModule::getOutputTypes()
{
    return {DATA_FILE, DATA_STREAM};
}

QPSKDemodModule::~QPSKDemodModule()
{
    delete[] sym_buffer;
}

void QPSKDemodModule::process()
{
    if (input_data_type == DATA_FILE)
        filesize = file_source->getFilesize();
    else
        filesize = 0;

    if (output_data_type == DATA_FILE)
    {
        data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".soft");
    }

    logger->info("Using input baseband " + d_input_file);
    logger->info("Demodulating to " + d_output_file_hint + ".soft");
    logger->info("Buffer size : " + std::to_string(d_buffer_size));

    time_t lastTime = 0;

    // Start
    if (input_data_type == DATA_FILE)
        file_source->start();
    if (d_dc_block)
        dcb->start();
    if (resample)
        res->start();
    agc->start();
    rrc->start();
    pll->start();
    rec->start();

    int dat_size = 0;
    while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
    {
        dat_size = rec->output_stream->read();

        if (dat_size <= 0)
            continue;

        // Estimate SNR, only on part of the samples to limit CPU usage
        snr_estimator.update(rec->output_stream->readBuf, dat_size / 100);
        snr = snr_estimator.snr();

        if (snr > peak_snr)
            peak_snr = snr;

        for (int i = 0; i < dat_size; i++)
        {
            sym_buffer[i * 2] = clamp(rec->output_stream->readBuf[i].real * 100);
            sym_buffer[i * 2 + 1] = clamp(rec->output_stream->readBuf[i].imag * 100);
        }

        rec->output_stream->flush();

        if (output_data_type == DATA_FILE)
            data_out.write((char *)sym_buffer, dat_size * 2);
        else
            output_fifo->write((uint8_t *)sym_buffer, dat_size * 2);

        // Update module stats
        module_stats["snr"] = snr;

        if (input_data_type == DATA_FILE)
            progress = file_source->getPosition();
        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
        }
    }

    logger->info("Demodulation finished");

    if (input_data_type == DATA_FILE)
        stop();
}

void QPSKDemodModule::stop()
{
    // Stop
    if (input_data_type == DATA_FILE)
        file_source->stop();
    if (d_dc_block)
        dcb->stop();
    if (resample)
        res->stop();
    agc->stop();
    rrc->stop();
    pll->stop();
    rec->stop();
    rec->output_stream->stopReader();

    if (output_data_type == DATA_FILE)
        data_out.close();
}

void QPSKDemodModule::drawUI(bool window)
{
    ImGui::Begin("QPSK Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

    ImGui::BeginGroup();
    // Constellation
    constellation.pushComplex(rec->output_stream->readBuf, rec->output_stream->getDataSize());
    constellation.draw();
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    {
        // Show SNR information
        ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
        snr_plot.draw(snr, peak_snr);
    }
    ImGui::EndGroup();

    if (!streamingInput)
        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

    ImGui::End();
}

std::string QPSKDemodModule::getID()
{
    return "qpsk_demod";
}

std::vector<std::string> QPSKDemodModule::getParameters()
{
    return {"samplerate", "symbolrate", "agc_rate", "rrc_alpha", "rrc_taps", "costas_bw", "iq_invert", "buffer_size", "dc_block", "baseband_format", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
}

std::shared_ptr<ProcessingModule> QPSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
{
    return std::make_shared<QPSKDemodModule>(input_file, output_file_hint, parameters);
}
