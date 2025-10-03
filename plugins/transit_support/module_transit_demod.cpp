#include "module_transit_demod.h"
#include "common/audio/audio_sink.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/io/wav_writer.h"
#include "core/config.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <volk/volk.h>

namespace transit
{
    TransitDemodModule::TransitDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        sym_buffer = new int8_t[d_buffer_size * 4];
        
        // Parse params
        if (parameters.count("save_wav") > 0)
            save_wav = parameters["save_wav"].get<bool>();
        if (parameters.count("save_soft") > 0)
            save_soft = parameters["save_soft"].get<bool>();

        if (parameters.count("clock_gain_omega") > 0)
            d_clock_gain_omega = parameters["clock_gain_omega"].get<float>();

        if (parameters.count("clock_mu") > 0)
            d_clock_mu = parameters["clock_mu"].get<float>();

        if (parameters.count("clock_gain_mu") > 0)
            d_clock_gain_mu = parameters["clock_gain_mu"].get<float>();

        if (parameters.count("clock_omega_relative_limit") > 0)
            d_clock_omega_relative_limit = parameters["clock_omega_relative_limit"].get<float>();

        name = "Transit Demodulator";
        play_audio = satdump::satdump_cfg.shouldPlayAudio();

        // constellation.d_hscale = 0.75; // 80.0 / 100.0;
        // constellation.d_vscale = 0.25; // 20.0 / 100.0;

        constellation.d_hscale = 0.1; // 80.0 / 100.0;
        constellation.d_vscale = 0.2; // 20.0 / 100.0;

        MIN_SPS = 1;
        MAX_SPS = 1000.0;
    }

    void TransitDemodModule::init()
    {
        BaseDemodModule::initb();

        // DSP implemented as per Xerbos blog posts, mostly the Part 1.5 GNU radio flowchart
        // https://xerbo.net/posts/investigating-transit-pt1/
        // https://xerbo.net/posts/investigating-transit-pt15/
        // Cannot currently be completely implemented due to lack of PLL Frequency Detector block in SatDump DSP, Quadrature demod gives practically identical results
        // The clock recovery tweaks could also not be completely replicated, but it still works alright
        /* DSP chain (old)
         * (c) = complex, (f) = float
         *                    [Audio output]<-(f)[Complex to Real]<-[Band pass filter]<─[Frequency shift PAM]<┐
         *                                                             ┌─>[Wav sink]                          │
         *  [input IQ](c)─>[Rational Resampler]─>[Quadrature demod](f)─┴─>[Real to complex](c)────────────────┤
         *                                                                                                    │
         *                 ┌─[Clock recovery]<─(f)[Quadrature demod]<-[Low pass filter]<─[Frequency shift FSK]┘
         *                 │
         *                 └─>[Soft Symbol output]
         */

        /* DSP chain (new)
         * (c) = complex, (f) = float
         *                      [Audio output]<-(f)[Complex to Real]<-[Band pass filter]<─[Frequency shift PAM]<─┐
         *                                                                                ┌─>[Wav sink]          │
         *  [input IQ](c)─>[Rational Resampler]─>[Carrier PLL]─>[complex to imaginary](f)─┴─>[Real to complex](c)┤
         *                                                                                                       │
         *        ┌─[Root Raised Cosine filter]<─(f)[Quadrature demod]<-[Low pass filter]<─[Frequency shift FSK]─┘
         *        │
         *        └─>[Clock recovery]─>[Soft Symbol output]
         */

        // Resampler to BW
        res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, d_symbolrate, final_samplerate);

        // PLL
        float d_carrier_pll_bw = 0.01;
        float d_carrier_pll_max_offset = 2*M_PI*10000/d_symbolrate;
        carrier_pll = std::make_shared<dsp::PLLCarrierTrackingBlock>(res->output_stream, d_carrier_pll_bw, d_carrier_pll_max_offset, -d_carrier_pll_max_offset);

        // Quadrature demod
        // qua = std::make_shared<dsp::QuadratureDemodBlock>(carrier_pll->output_stream, dsp::hz_to_rad(12000, d_symbolrate));

        // split real demod portion to wav and demod
        demod_stream = std::make_shared<dsp::stream<float>>();

        // Convert demod'd audio back to complex for FSK demodulation
        rtc = std::make_shared<dsp::RealToComplexBlock>(demod_stream);

        double fsk_sym_rate = 4999600/12800;
        double fsk_sps = d_symbolrate / fsk_sym_rate;

        logger->info("FSK SPS: %f", fsk_sps);

        // FSK demod
        fsk_stream = std::make_shared<dsp::stream<complex_t>>();
        fsk_fsb = std::make_shared<dsp::FreqShiftBlock>(fsk_stream, d_symbolrate, -10640);
        fsk_lpf = std::make_shared<dsp::FIRBlock<complex_t>>(fsk_fsb->output_stream, dsp::firdes::low_pass(1.0, d_symbolrate, 1200, 1000));
        fsk_qua = std::make_shared<dsp::QuadratureDemodBlock>(fsk_lpf->output_stream, dsp::hz_to_rad(600*2, d_symbolrate));
        fsk_rrc = std::make_shared<dsp::FIRBlock<float>>(fsk_qua->output_stream, dsp::firdes::root_raised_cosine(1/(2*M_PI*630/d_symbolrate), d_symbolrate, fsk_sym_rate, 0.35, 401));
        fsk_rec = std::make_shared<dsp::GardnerClockRecoveryBlock<float>>(fsk_rrc->output_stream, fsk_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);

        // USB demod to play the "music" to the user
        // values picked to make PAM sound best
        usb_stream = std::make_shared<dsp::stream<complex_t>>();
        usb_fsb = std::make_shared<dsp::FreqShiftBlock>(usb_stream, d_symbolrate, -4600);
        usb_bpf = std::make_shared<dsp::FIRBlock<complex_t>>(usb_fsb->output_stream, dsp::firdes::band_pass(1, d_symbolrate, 0, 4000, 500, dsp::fft::window::WIN_KAISER));
    }

    TransitDemodModule::~TransitDemodModule() { delete[] sym_buffer; }

    void TransitDemodModule::process()
    {
        if (input_data_type == satdump::pipeline::DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        logger->info("Using input baseband " + d_input_file);

        if (save_wav)
        {
            wav_out = std::ofstream(d_output_file_hint + ".wav", std::ios::binary);
            d_output_file = d_output_file_hint + ".wav";
            logger->info("Demodulating to " + d_output_file_hint + ".wav");
        }

        if (save_soft || output_data_type == satdump::pipeline::DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
            d_output_file = d_output_file_hint + ".soft";
            logger->info("Demodulating to " + d_output_file_hint + ".soft");
        }

        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        res->start();
        carrier_pll->start();
        rtc->start();

        fsk_fsb->start();
        fsk_lpf->start();
        fsk_qua->start();
        fsk_rrc->start();
        fsk_rec->start();

        std::shared_ptr<audio::AudioSink> audio_sink;
        if (input_data_type != satdump::pipeline::DATA_FILE && audio::has_sink())
        {
            enable_audio = true;
            audio_sink = audio::get_default_sink();
            audio_sink->set_samplerate(d_symbolrate);
            audio_sink->start();

            usb_fsb->start();
            usb_bpf->start();
        }

        // Buffers to wav
        float *output_real = new float[d_buffer_size * 100];

        // zero buffer for interlacing real to complex
        float *zero_buf = new float[d_buffer_size * 100];
        memset(zero_buf, 0, d_buffer_size * 100 * sizeof(float));

        complex_t *output_snr_complex = new complex_t[d_buffer_size * 100];

        // these could be deduplicated
        int16_t *output_wav_buffer = new int16_t[d_buffer_size * 100];
        int16_t *output_audio_buffer = new int16_t[d_buffer_size * 100];
        
        uint64_t final_wav_size = 0;

        dsp::WavWriter wave_writer(wav_out);
        if (save_wav)
            wave_writer.write_header(d_symbolrate, 1);

        int dat_size = 0;
        while (demod_should_run())
        {
            // handle DSP
            {
                // demod split/Wav save
                int dat_size = carrier_pll->output_stream->read();

                if(dat_size <= 0)
                {
                    carrier_pll->output_stream->flush();
                }
                else
                {
                    // convert complex to imaginary
                    volk_32fc_deinterleave_imag_32f(output_real, (lv_32fc_t*)carrier_pll->output_stream->readBuf, dat_size);

                    // save WAV
                    if (save_wav)
                    {
                        for (int i = 0; i < dat_size; i++)
                        {
                            if (output_real[i] > 1.0f)
                                output_real[i] = 1.0f;
                            if (output_real[i] < -1.0f)
                                output_real[i] = -1.0f;
                        }

                        volk_32f_s32f_convert_16i(output_wav_buffer, demod_stream->writeBuf, 32767, dat_size);

                        wav_out.write((char *)output_wav_buffer, dat_size * sizeof(int16_t));
                        final_wav_size += dat_size * sizeof(int16_t);
                    }

                    // forward to demods
                    memcpy(demod_stream->writeBuf, output_real, dat_size * sizeof(float));
                    demod_stream->swap(dat_size);
                    carrier_pll->output_stream->flush();
                }

                // FSK/USB split
                dat_size = rtc->output_stream->read();

                if(dat_size <= 0)
                {
                    rtc->output_stream->flush();
                }
                else
                {
                    // forward to FSK
                    memcpy(fsk_stream->writeBuf, rtc->output_stream->readBuf, dat_size * sizeof(complex_t));
                    fsk_stream->swap(dat_size);

                    // forward to USB
                    if(enable_audio)
                    {
                        memcpy(usb_stream->writeBuf, rtc->output_stream->readBuf, dat_size * sizeof(complex_t));
                        usb_stream->swap(dat_size);
                    }
                    
                    rtc->output_stream->flush();
                }
            }

            // FSK demod
            dat_size = fsk_rec->output_stream->read();

            if (dat_size <= 0)
            {
                fsk_rec->output_stream->flush();
            }
            else
            {
                // Into const
                constellation.pushFloatAndGaussian(fsk_rec->output_stream->readBuf, dat_size);

                volk_32f_x2_interleave_32fc((lv_32fc_t*)output_snr_complex, fsk_rec->output_stream->readBuf, zero_buf, dat_size);
                snr_estimator.update(output_snr_complex, dat_size);
                snr = snr_estimator.snr();

                if (snr > peak_snr)
                    peak_snr = snr;

                display_freq = dsp::rad_to_hz(carrier_pll->getFreq(), d_symbolrate);

                for (int i = 0; i < dat_size; i++)
                {
                    sym_buffer[i] = clamp(fsk_rec->output_stream->readBuf[i] * 5); // TODO
                }

                fsk_rec->output_stream->flush();

                if (save_soft || output_data_type == satdump::pipeline::DATA_FILE)
                    data_out.write((char *)sym_buffer, dat_size);
                if (output_data_type != satdump::pipeline::DATA_FILE)
                    output_fifo->write((uint8_t *)sym_buffer, dat_size);

                if (input_data_type == satdump::pipeline::DATA_FILE)
                    progress = file_source->getPosition();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            // audio output
            if(enable_audio)
            {
                int dat_size_audio = usb_bpf->output_stream->read();
                if (dat_size_audio <= 0)
                {
                    usb_bpf->output_stream->flush();
                }
                else
                {
                    if (play_audio)
                    {
                        volk_32fc_deinterleave_real_32f(output_real, (lv_32fc_t *)usb_bpf->output_stream->readBuf, dat_size_audio);
                        volk_32f_s32f_convert_16i(output_audio_buffer, output_real, 32767, dat_size_audio);
                        audio_sink->push_samples(output_audio_buffer, dat_size_audio);
                    }
                    usb_bpf->output_stream->flush();
                }
            }
        }

        if (enable_audio)
            audio_sink->stop();

        // Finish up WAV
        if (save_wav)
        {
            wave_writer.finish_header(final_wav_size);
            wav_out.close();
        }
        delete[] output_wav_buffer;

        // close soft sym file
        if(save_soft || output_data_type == satdump::pipeline::DATA_FILE)
        {
            data_out.close();
        }

        logger->info("Demodulation finished");

        if (input_data_type == satdump::pipeline::DATA_FILE)
            stop();
    }

    nlohmann::json TransitDemodModule::getModuleStats()
    {
        nlohmann::json v;
        v["progress"] = ((double)progress / (double)filesize);
        v["snr"] = snr;
        v["peak_snr"] = peak_snr;
        return v;
    }

    void TransitDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        res->stop();
        carrier_pll->stop();
        carrier_pll->output_stream->stopReader();
        rtc->stop();
        rtc->output_stream->stopReader();

        fsk_fsb->stop();
        fsk_lpf->stop();
        fsk_qua->stop();
        fsk_rrc->stop();
        fsk_rec->stop();
        fsk_rec->output_stream->stopReader();

        if(enable_audio)
        {
            usb_fsb->stop();
            usb_bpf->stop();
            usb_bpf->output_stream->stopReader();
        }
    }

    void TransitDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});

            ImGui::Text("Freq : ");
            ImGui::SameLine();
            ImGui::TextColored(style::theme.orange, "%.0f Hz", display_freq);

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

            snr_plot.draw(snr, peak_snr);
            if (!d_is_streaming_input)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_enabled("fft", show_fft);
        }
        ImGui::EndGroup();

        if (!d_is_streaming_input)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        drawStopButton();
        ImGui::End();
        drawFFT();
    }

    std::string TransitDemodModule::getID() { return "transit_demod"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> TransitDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TransitDemodModule>(input_file, output_file_hint, parameters);
    }
} // namespace transit