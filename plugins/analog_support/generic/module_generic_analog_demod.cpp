#include "module_generic_analog_demod.h"
#include "common/dsp/block.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "common/dsp/utils/complex_to_mag.h"
#include "common/dsp/utils/freq_shift.h"
#include "common/dsp_source_sink/format_notated.h"
#include "core/config.h"
#include "core/module.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <complex.h>
#include <cstdint>
#include <memory>
#include <volk/volk.h>
#include <volk/volk_complex.h>
#include "common/dsp/io/wav_writer.h"
#include "common/audio/audio_sink.h"

namespace generic_analog
{
    GenericAnalogDemodModule::GenericAnalogDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        name = "Generic Analog Demodulator (WIP)";
        show_freq = true;
        play_audio = satdump::config::main_cfg["user_interface"]["play_audio"]["value"].get<bool>();

        constellation.d_hscale = 1.0; // 80.0 / 100.0;
        constellation.d_vscale = 0.5; // 20.0 / 100.0;

        MIN_SPS = 1;
        MAX_SPS = 1e9;

        upcoming_symbolrate = d_symbolrate;
    }

    void GenericAnalogDemodModule::init()
    {
        BaseDemodModule::initb();

        // Resampler to BW
        //res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, d_symbolrate, final_samplerate);

	// Frequency shift - LSB
	//fsb = std::make_shared<dsp::FreqShiftBlock>(res->output_stream, final_samplerate, -3200);

	// Low pass filter
	//lpf = std::make_shared<dsp::FIRBlock<complex_t>>(fsb->output_stream, dsp::firdes::low_pass(1.0, final_samplerate, 4000, 1000));


        // Quadrature demod
        //qua = std::make_shared<dsp::QuadratureDemodBlock>(res->output_stream, dsp::hz_to_rad(d_symbolrate / 2, d_symbolrate));


	bpf = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::band_pass(1, d_samplerate, 1000, 400000, 2000));

	// Frequency shift
	//fsb = std::make_shared<dsp::FreqShiftBlock>(res->output_stream, final_samplerate, -3200);

	// Pll
	//pll = std::make_shared<dsp::PLLCarrierTrackingBlock>(bpf->output_stream, d_pll_bw, d_pll_max_offset, -d_pll_max_offset);

	// AM demod
	//fsb = std::make_shared<dsp::ComplexToMagBlock>(pll->output_stream);
    }

    GenericAnalogDemodModule::~GenericAnalogDemodModule()
    {
    }

    void GenericAnalogDemodModule::process()
    {


	if (input_data_type == DATA_FILE)
		filesize = file_source->getFilesize();
	else
		filesize = 0;

	if (output_data_type == DATA_FILE)
	{
		data_out = std::ofstream(d_output_file_hint + ".f32", std::ios::binary);
		d_output_files.push_back(d_output_file_hint + ".f32");
	}
	
	logger->info("Using input baseband" + d_input_file);
	logger->info("Saving processed to " + d_output_file_hint + ".f32");
	logger->info("Buffer size : " + std::to_string(d_buffer_size));

	time_t lastTime = 0;
        //if (input_data_type == DATA_FILE)
        //    filesize = file_source->getFilesize();
        //else
        //    filesize = 0;

        //if (output_data_type == DATA_FILE)
        //{
        //    data_out = std::ofstream(d_output_file_hint + ".wav", std::ios::binary);
        //    d_output_files.push_back(d_output_file_hint + ".wav");
        //}

        //logger->info("Using input baseband " + d_input_file);
        //logger->info("Demodulating to " + d_output_file_hint + ".wav");
        //logger->info("Buffer size : " + std::to_string(d_buffer_size));

        //time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        //res->start();
	//pll->start();
	//fsb->start();
	//fsb->start();
	//qua->start();
	bpf->start();

        // Buffers to wav
        //int16_t *output_wav_buffer = new int16_t[d_buffer_size * 100];
        //int16_t *output_wav_buffer_resamp = new int16_t[d_buffer_size * 200];
        uint64_t final_data_size = 0;
        //dsp::WavWriter wave_writer(data_out);
        //if (output_data_type == DATA_FILE)
        //    wave_writer.write_header(audio_samplerate, 1);

        //std::shared_ptr<audio::AudioSink> audio_sink;
        //if (input_data_type != DATA_FILE && audio::has_sink())
        //{
        //    enable_audio = false;
        //    audio_sink = audio::get_default_sink();
        //    audio_sink->set_samplerate(audio_samplerate);
        //    audio_sink->start();
        //}

        /////////////
        //dsp::RationalResamplerBlock<complex_t> input_resamp(nullptr, d_symbolrate, final_samplerate);
        //dsp::QuadratureDemodBlock quad_demod(nullptr, dsp::hz_to_rad(d_symbolrate / 2, d_symbolrate));
	//dsp::PLLCarrierTrackingBlock pll(nullptr, d_pll_bw, d_pll_max_offset, -d_pll_max_offset);
        complex_t *work_buffer_complex = dsp::create_volk_buffer<complex_t>(d_buffer_size);
        float *work_buffer_float = dsp::create_volk_buffer<float>(d_buffer_size);
        float *work_buffer_float2 = dsp::create_volk_buffer<float>(d_buffer_size);

        int dat_size = 0;
        while (demod_should_run())
        {
            dat_size = bpf->output_stream->read();

            if (dat_size <= 0)
            {
                bpf->output_stream->flush();
                continue;
            }
/*
#if 1
            proc_mtx.lock();

            if (settings_changed)
            {
                if (upcoming_symbolrate > 0)
                {
                    d_symbolrate = upcoming_symbolrate;
                    input_resamp.set_ratio(d_symbolrate, final_samplerate);
                    quad_demod.set_gain(dsp::hz_to_rad(d_symbolrate / 2, d_symbolrate));
                }

                switch (e) {
                    case 0:
                        nfm_demod = true;
                        am_demod = false;
                        break;
                    case 1:
                        am_demod = true;
                        nfm_demod = false;
                        break;
                    default:
                        nfm_demod = true;
                        am_demod = false;
                        break;
                }
                
                settings_changed = false;
            }

            int nout = input_resamp.process(agc->output_stream->readBuf, dat_size, work_buffer_complex);
            
            if (nfm_demod)
            {
                nout = quad_demod.process(work_buffer_complex, nout, work_buffer_float);
            }

            if (am_demod)
            {
                volk_32fc_magnitude_32f((float *)work_buffer_float, (lv_32fc_t *)work_buffer_complex, nout);
	    }

            {
                
                // Into const
                constellation.pushFloatAndGaussian(work_buffer_float, nout);

                for (int i = 0; i < nout; i++)
                {
                    if (work_buffer_float[i] > 1.0f)
                        work_buffer_float[i] = 1.0f;
                    if (work_buffer_float[i] < -1.0f)
                        work_buffer_float[i] = -1.0f;
                }

                volk_32f_s32f_convert_16i(output_wav_buffer, (float *)work_buffer_float, 65535 * 0.68, nout);

                int final_out = audio::AudioSink::resample_s16(output_wav_buffer, output_wav_buffer_resamp, d_symbolrate, audio_samplerate, nout, 1);
                if (enable_audio && play_audio)
                    audio_sink->push_samples(output_wav_buffer_resamp, final_out);
                if (output_data_type == DATA_FILE)
                {
                    data_out.write((char *)output_wav_buffer_resamp, final_out * sizeof(int16_t));
                    final_data_size += final_out * sizeof(int16_t);
                }
                else
                {
                    output_fifo->write((uint8_t *)output_wav_buffer_resamp, final_out * sizeof(int16_t));
                }
            }

            proc_mtx.unlock();
#else
	    */
            // Into const
	    constellation.pushComplex(bpf->output_stream->readBuf, bpf->output_stream->getDataSize());
        
	    //constellation.pushFloatAndGaussian(bpf->output_stream->readBuf, bpf->output_stream->getDataSize());

            //for (int i = 0; i < dat_size; i++)
            //{
            //    if (bpf->output_stream->readBuf[i] > 1.0f)
            //        bpf->output_stream->readBuf[i] = 1.0f;
            //    if (bpf->output_stream->readBuf[i] < -1.0f)
            //        bpf->output_stream->readBuf[i] = -1.0f;
            //}

	    //display_freq = dsp::rad_to_hz(pll->getFreq(), final_samplerate);

	    //volk_32fc_deinterleave_real_32f((float *)work_buffer_float, (lv_32fc_t *)bpf->output_stream->readBuf, dat_size);



	    if (output_data_type == DATA_FILE)
	    {
		    data_out.write((char *)bpf->output_stream->readBuf, dat_size * sizeof(complex_t));
		    final_data_size += dat_size * sizeof(complex_t);
	    }
	    else 
	    {
	    output_fifo->write((uint8_t *)bpf->output_stream->readBuf, dat_size * sizeof(complex_t));
	    }

	    //////// just to test the raw output
            ////volk_32f_s32f_convert_16i(output_wav_buffer, (float *)work_buffer_float, 65535 * 0.68, dat_size);

            ////int final_out = audio::AudioSink::resample_s16(output_wav_buffer, output_wav_buffer_resamp, d_symbolrate, audio_samplerate, dat_size, 1);
            ////if (enable_audio && play_audio)
            ////    audio_sink->push_samples(output_wav_buffer_resamp, final_out);
            ////if (output_data_type == DATA_FILE)
            ////{
            ////    data_out.write((char *)output_wav_buffer_resamp, final_out * sizeof(int16_t));
            ////    final_data_size += final_out * sizeof(int16_t);
            ////}
            ////else
            ////{
            ////    output_fifo->write((uint8_t *)output_wav_buffer_resamp, final_out * sizeof(int16_t));
            ////}
//#endif

            bpf->output_stream->flush();

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        //if (enable_audio)
        //    audio_sink->stop();

        //// Finish up WAV
        //if (output_data_type == DATA_FILE)
        //{
        //    wave_writer.finish_header(final_data_size);
        //    data_out.close();
        //}

        //delete[] output_wav_buffer;
        //delete[] output_wav_buffer_resamp;

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void GenericAnalogDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        //res->stop();
	//pll->stop();
	//qua->stop();
	bpf->stop();
        bpf->output_stream->stopReader();
    }

    void GenericAnalogDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Settings", {200 * ui_scale, 20 * ui_scale});

            proc_mtx.lock();
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputInt("Bandwidth##bandwidthsetting", &upcoming_symbolrate);

            ImGui::RadioButton("NFM##analogoption", &e, 0);
            ImGui::SameLine();
            ImGui::RadioButton("AM##analogoption", &e, 1);
            style::beginDisabled();
            ImGui::RadioButton("WFM##analogoption", false);
            ImGui::SameLine();
            ImGui::RadioButton("USB##analogoption", false);
            //ImGui::SameLine();
            ImGui::RadioButton("LSB##analogoption", false);
            // ImGui::SameLine();
            ImGui::SameLine();
            ImGui::RadioButton("CW##analogoption", false);
            style::endDisabled();

            if (ImGui::Button("Set###analogset"))
                settings_changed = true;
            proc_mtx.unlock();

            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
	    if (show_freq)
            {
                ImGui::Text("Freq : ");
                ImGui::SameLine();
                ImGui::TextColored(style::theme.orange, "%s", format_notated(display_freq, "Hz", 4).c_str());
            }
            snr_plot.draw(snr, peak_snr);
            if (!streamingInput)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_enabled("fft", show_fft);
            //if (enable_audio)
            //{
            //    const char *btn_icon, *label;
            //    ImVec4 color;
            //    if (play_audio)
            //    {
            //        color = style::theme.green.Value;
            //        btn_icon = u8"\uF028##aptaudio";
            //        label = "Audio Playing";
            //    }
            //    else
            //    {
            //        color = style::theme.red.Value;
            //        btn_icon = u8"\uF026##aptaudio";
            //        label = "Audio Muted";
            //    }

            //    ImGui::PushStyleColor(ImGuiCol_Text, color);
            //    if (ImGui::Button(btn_icon))
            //        play_audio = !play_audio;
            //    ImGui::PopStyleColor();
            //    ImGui::SameLine();
            //    ImGui::TextUnformatted(label);
            //}
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        drawStopButton();
        ImGui::End();
        drawFFT();
    }

    std::string GenericAnalogDemodModule::getID()
    {
        return "generic_analog_demod";
    }

    std::vector<std::string> GenericAnalogDemodModule::getParameters()
    {
        std::vector<std::string> params;
        return params;
    }

    std::shared_ptr<ProcessingModule> GenericAnalogDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GenericAnalogDemodModule>(input_file, output_file_hint, parameters);
    }
}
