#include "module_terra_db_demod.h"
#include "common/dsp/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace terra
{
    TerraDBDemodModule::TerraDBDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                              d_samplerate(parameters["samplerate"].get<long>()),
                                                                                                                              d_buffer_size(parameters["buffer_size"].get<long>()),
                                                                                                                              d_dc_block(parameters.count("dc_block") > 0 ? parameters["dc_block"].get<bool>() : 0),
                                                                                                                              constellation(0.5f, 0.5f, demod_constellation_size)
    {
        // Init DSP blocks
        file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(parameters["baseband_format"]), d_buffer_size);
        agc = std::make_shared<dsp::AGCBlock>(file_source->output_stream, 1e-3, 1.0f, 1.0f, 65536);
        rrc = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, dsp::firdes::root_raised_cosine(1, d_samplerate, 13.125e6 * 2, 0.5f, 31));
        pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, 0.004, 2);
        rec = std::make_shared<dsp::CCMMClockRecoveryBlock>(pll->output_stream, ((float)d_samplerate / (float)13.125e6) / 2.0f, pow(0.001, 2) / 4.0, 0.5f, 0.001, 0.0001f);

        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 2];
        snr = 0;
        peak_snr = 0;
    }

    std::vector<ModuleDataType> TerraDBDemodModule::getInputTypes()
    {
        return {DATA_FILE};
    }

    std::vector<ModuleDataType> TerraDBDemodModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    TerraDBDemodModule::~TerraDBDemodModule()
    {
        delete[] sym_buffer;
    }

    void TerraDBDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        //if (input_data_type == DATA_FILE)
        //    data_in = std::ifstream(d_input_file, std::ios::binary);

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
        file_source->start();
        agc->start();
        rrc->start();
        pll->start();
        rec->start();

        int dat_size = 0;
        while (/*input_data_type == DATA_STREAM ? input_active.load() : */ !file_source->eof())
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
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i].real * 50);
            }

            rec->output_stream->flush();

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, dat_size);
            else
                output_fifo->write((uint8_t *)sym_buffer, dat_size);

            progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
            }
        }

        logger->info("Demodulation finished");

        // Stop
        file_source->stop();
        agc->stop();
        rrc->stop();
        pll->stop();
        rec->stop();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void TerraDBDemodModule::drawUI(bool window)
    {
        ImGui::Begin("Terra DB Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
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

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string TerraDBDemodModule::getID()
    {
        return "terra_db_demod";
    }

    std::vector<std::string> TerraDBDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "dc_block", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> TerraDBDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TerraDBDemodModule>(input_file, output_file_hint, parameters);
    }
}
