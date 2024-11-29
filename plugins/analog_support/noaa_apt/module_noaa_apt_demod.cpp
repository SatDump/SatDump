#include "module_noaa_apt_demod.h"
#include "common/dsp/filter/firdes.h"
#include "core/config.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "common/dsp/io/wav_writer.h"
#include "common/audio/audio_sink.h"

namespace noaa_apt
{
    NOAAAPTDemodModule::NOAAAPTDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Parse params
        if (parameters.count("sdrpp_noise_reduction") > 0)
            sdrpp_noise_reduction = parameters["sdrpp_noise_reduction"].get<bool>();
        if (parameters.count("save_wav") > 0)
            save_wav = parameters["save_wav"].get<bool>();

        name = "NOAA APT Demodulator (FM)";
        show_freq = false;
        play_audio = satdump::config::main_cfg["user_interface"]["play_audio"]["value"].get<bool>();

        constellation.d_hscale = 1.0; // 80.0 / 100.0;
        constellation.d_vscale = 0.5; // 20.0 / 100.0;

        MIN_SPS = 1;
        MAX_SPS = 1000.0;
    }

    void NOAAAPTDemodModule::init()
    {
        BaseDemodModule::initb();

        // Resampler to BW
        res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, d_symbolrate, final_samplerate);

        // Noise reduction
        if (sdrpp_noise_reduction)
            nr = std::make_shared<dsp::AptNoiseReductionBlock>(res->output_stream, 9);

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(sdrpp_noise_reduction ? nr->output_stream : res->output_stream, dsp::hz_to_rad(d_symbolrate / 2, d_symbolrate));
    }

    NOAAAPTDemodModule::~NOAAAPTDemodModule()
    {
    }

    void NOAAAPTDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (save_wav || output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".wav", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".wav");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".wav");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        res->start();
        if (sdrpp_noise_reduction)
            nr->start();
        qua->start();

        // Buffers to wav
        int16_t *output_wav_buffer = new int16_t[d_buffer_size * 100];
        uint64_t final_data_size = 0;
        dsp::WavWriter wave_writer(data_out);
        if (save_wav || output_data_type == DATA_FILE)
            wave_writer.write_header(d_symbolrate, 1);

        std::shared_ptr<audio::AudioSink> audio_sink;
        if (input_data_type != DATA_FILE && audio::has_sink())
        {
            enable_audio = true;
            audio_sink = audio::get_default_sink();
            audio_sink->set_samplerate(d_symbolrate);
            audio_sink->start();
        }

        int dat_size = 0;
        while (demod_should_run())
        {
            dat_size = qua->output_stream->read();

            if (dat_size <= 0)
            {
                qua->output_stream->flush();
                continue;
            }

            // Into const
            constellation.pushFloatAndGaussian(qua->output_stream->readBuf, qua->output_stream->getDataSize());

            for (int i = 0; i < dat_size; i++)
            {
                if (qua->output_stream->readBuf[i] > 1.0f)
                    qua->output_stream->readBuf[i] = 1.0f;
                if (qua->output_stream->readBuf[i] < -1.0f)
                    qua->output_stream->readBuf[i] = -1.0f;
            }

            volk_32f_s32f_convert_16i(output_wav_buffer, (float *)qua->output_stream->readBuf, 32767, dat_size);

            if (enable_audio && play_audio)
                audio_sink->push_samples(output_wav_buffer, dat_size);

            if (save_wav || output_data_type == DATA_FILE)
            {
                data_out.write((char *)output_wav_buffer, dat_size * sizeof(int16_t));
                final_data_size += dat_size * sizeof(int16_t);
            }
            if(output_data_type != DATA_FILE)
            {
                output_fifo->write((uint8_t *)output_wav_buffer, dat_size * sizeof(int16_t));
            }

            qua->output_stream->flush();

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        if (enable_audio)
            audio_sink->stop();

        // Finish up WAV
        if (save_wav || output_data_type == DATA_FILE)
        {
            wave_writer.finish_header(final_data_size);
            data_out.close();
        }
        delete[] output_wav_buffer;

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void NOAAAPTDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        res->stop();
        if (sdrpp_noise_reduction)
            nr->stop();
        qua->stop();
        qua->output_stream->stopReader();
    }

    void NOAAAPTDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            /* if (show_freq)
            {
                ImGui::Text("Freq : ");
                ImGui::SameLine();
                ImGui::TextColored(style::theme.orange, "%.0f Hz", display_freq);
            }
            snr_plot.draw(snr, peak_snr); */
            if (!streamingInput)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_enabled("fft", show_fft);
            if (enable_audio)
            {
                const char *btn_icon, *label;
                ImVec4 color;
                if (play_audio)
                {
                    color = style::theme.green.Value;
                    btn_icon = u8"\uF028##aptaudio";
                    label = "Audio Playing";
                }
                else
                {
                    color = style::theme.red.Value;
                    btn_icon = u8"\uF026##aptaudio";
                    label = "Audio Muted";
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                if (ImGui::Button(btn_icon))
                    play_audio = !play_audio;
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextUnformatted(label);
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        drawStopButton();
        ImGui::End();
        drawFFT();
    }

    std::string NOAAAPTDemodModule::getID()
    {
        return "noaa_apt_demod";
    }

    std::vector<std::string> NOAAAPTDemodModule::getParameters()
    {
        std::vector<std::string> params;
        return params;
    }

    std::shared_ptr<ProcessingModule> NOAAAPTDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAAAPTDemodModule>(input_file, output_file_hint, parameters);
    }
}