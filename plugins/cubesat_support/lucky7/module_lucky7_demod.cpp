#include "module_lucky7_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "common/utils.h"
#include "common/scrambling.h"

#define FRM_SIZE 440

namespace lucky7
{
    Lucky7DemodModule::Lucky7DemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : demod::BaseDemodModule(input_file, output_file_hint, parameters)
    {
        name = "Lucky-7 Demodulator";
        show_freq = false;

        corr_thresold = parameters["corr_thresold"];

        constellation.d_hscale = 80.0 / 100.0;
        constellation.d_vscale = 20.0 / 100.0;
    }

    void Lucky7DemodModule::init()
    {
        BaseDemodModule::init();

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(agc->output_stream, 1.0f);

        // DC Blocker
        dcb2 = std::make_shared<dsp::CorrectIQBlock<float>>(qua->output_stream);

        // LPF
        std::vector<float> taps;
        for (int i = 0; i < final_sps; i++)
            taps.push_back(0.1f);
        fil = std::make_shared<dsp::FIRBlock<float>>(dcb2->output_stream, taps);

        // Buffers
        std::vector<float> sync = {-1, -1, 1, -1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, -1, -1};
        sync_buf = oversample_vector(sync, final_sps);

        oversampled_size = FRM_SIZE * final_sps;
        shifting_buffer = new float[oversampled_size];
    }

    Lucky7DemodModule::~Lucky7DemodModule()
    {
        if (shifting_buffer != nullptr)
            delete[] shifting_buffer;
    }

    void Lucky7DemodModule::process_sample(float &news)
    {
        const int offset = 0;
        int sps = final_sps;

        memmove(&shifting_buffer[0], &shifting_buffer[1], (oversampled_size - 1) * sizeof(float));
        shifting_buffer[oversampled_size - 1] = news;

        if (to_skip_samples > 0)
        {
            to_skip_samples--;
            return;
        }

        float corr_value = 0;
        volk_32f_x2_dot_prod_32f(&corr_value, &shifting_buffer[offset], sync_buf.data(), sync_buf.size());

        if (corr_value > corr_thresold)
        {
            // printf("Corr %f\n", corr_value);

            float avg_frame[312];

            // Average to get symbols
            for (int i = 0; i < 312; i++)
            {
                float v = 0;
                for (int x = 0; x < sps; x++)
                    v += shifting_buffer[offset + i * sps + x];
                v /= sps;
                avg_frame[i] = v;
            }

            // Ensure it's centered, DC block can be too slow
            float total_avg = 0;
            for (int i = 0; i < 312; i++)
                total_avg += avg_frame[i];
            total_avg /= 312;
            for (int i = 0; i < 312; i++)
                avg_frame[i] -= total_avg;

            // To bytes
            uint8_t bits[312];
            for (int i = 0; i < 312; i++)
                bits[i / 8] = bits[i / 8] << 1 | (avg_frame[i] > 0);

            cubesat::scrambling::si4462_scrambling(&bits[2], 37);

            uint16_t crc_frm1 = crc_check.compute(&bits[2], 35);
            uint16_t crc_frm2 = bits[2 + 35 + 0] << 8 | bits[2 + 35 + 1];

            if (crc_frm1 == crc_frm2)
            {
                data_out.write((char *)&bits[2], 35);
                frm_cnt++;

                // Skip to next
                to_skip_samples = 4400 - 1;
            }
        }
    }

    void Lucky7DemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".frm");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        qua->start();
        dcb2->start();
        fil->start();

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = fil->output_stream->read();

            if (dat_size <= 0)
            {
                fil->output_stream->flush();
                continue;
            }

            // Into const
            constellation.pushFloatAndGaussian(fil->output_stream->readBuf, fil->output_stream->getDataSize());

            for (int i = 0; i < dat_size; i++)
                process_sample(fil->output_stream->readBuf[i]);

            fil->output_stream->flush();

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;

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

    void Lucky7DemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        qua->stop();
        dcb2->stop();
        fil->stop();
        fil->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void Lucky7DemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
#if 0
            // Show SNR information
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});

            snr_plot.draw(snr, peak_snr);
            if (!streamingInput)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_output_2nd(show_fft);
#endif

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");
                ImGui::SameLine();
                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frm_cnt));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();

        drawFFT();
    }

    std::string Lucky7DemodModule::getID()
    {
        return "lucky7_demod";
    }

    std::vector<std::string> Lucky7DemodModule::getParameters()
    {
        std::vector<std::string> params = {"corr_thresold"};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> Lucky7DemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Lucky7DemodModule>(input_file, output_file_hint, parameters);
    }
}