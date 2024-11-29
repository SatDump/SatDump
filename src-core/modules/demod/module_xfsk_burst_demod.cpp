#include "module_xfsk_burst_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// #include "common/audio/audio_sink.h"

namespace demod
{
    XFSKBurstDemodModule::XFSKBurstDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : BaseDemodModule(input_file, output_file_hint, parameters),
          d_deviation(parameters.contains("fsk_deviation") ? parameters["fsk_deviation"].get<float>() : 5e3)
    {
        name = "xFSK Burst Demodulator";
        show_freq = false;

        constellation.d_hscale = (80.0 / 10.0) / 100.0;
        constellation.d_vscale = 20.0 / 100.0;

        // MIN_SPS = 5; // 8;
        // MAX_SPS = 5; // 32.0;

        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 4];
    }

    /*
    The values for recovery here are from old GRC flowchart I had around.
    Not sure who was the original author, but as they work it wasn't worth re-doing all the work.
    */
    void XFSKBurstDemodModule::init()
    {
        BaseDemodModule::initb(false);

        // LPF1
        float carson_cuttoff = d_deviation + d_symbolrate / 2.0;
        lpf1 = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::low_pass(1.0, d_samplerate, carson_cuttoff, 2000));

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(lpf1->output_stream, d_samplerate / (2.0 * M_PI * d_deviation));

        // Resampling
        resamplerf = std::make_shared<dsp::SmartResamplerBlock<float>>(qua->output_stream, final_samplerate, d_samplerate);

        // AGC2
        agc2 = std::make_shared<dsp::AGC2Block<float>>(resamplerf->output_stream, 5.0, 0.01, 0.001);

        // LPF2
        lpf2 = std::make_shared<dsp::FIRBlock<float>>(agc2->output_stream, dsp::firdes::low_pass(1.0, final_samplerate, d_symbolrate / 2, 2000));

        // rec = std::make_shared<dsp::GardnerClockRecovery2Block>(agc2->output_stream, final_samplerate, d_symbolrate, 0.707, 24);
        rec = std::make_shared<dsp::GardnerClockRecoveryBlock<float>>(lpf2->output_stream, final_sps, (final_sps * M_PI) / 100.0, 0.5, 0.5 / 8.0, 0.01);
        //  rec = std::make_shared<dsp::MMClockRecoveryBlock<float>>(agc2->output_stream, final_sps, 0.0689062, 0.5, 0.525, 0.01);
    }

    XFSKBurstDemodModule::~XFSKBurstDemodModule()
    {
        delete[] sym_buffer;
    }

    void XFSKBurstDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".soft");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        lpf1->start();
        qua->start();
        agc2->start();
        lpf2->start();
        resamplerf->start();
        rec->start();

        ////////////////////////////////////////////////////////////////
        /*std::shared_ptr<audio::AudioSink> audio_sink;
        if (input_data_type != DATA_FILE && audio::has_sink())
        {
            logger->critical(final_samplerate);
            audio_sink = audio::get_default_sink();
            audio_sink->set_samplerate(final_samplerate);
            audio_sink->start();
        }

        int16_t *output_wav_buffer = new int16_t[d_buffer_size * 4];*/
        ////////////////////////////////////////////////////////////////

        int dat_size = 0;
        while (demod_should_run())
        {
#if 1
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
            {
                rec->output_stream->flush();
                continue;
            }

            // Into const
            constellation.pushFloatAndGaussian(rec->output_stream->readBuf, dat_size);

            // Estimate SNR
            snr_estimator.update((complex_t *)rec->output_stream->readBuf, dat_size / 2);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            for (int i = 0; i < dat_size; i++)
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i] * 10);

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, dat_size);
            else
                output_fifo->write((uint8_t *)sym_buffer, dat_size);

            rec->output_stream->flush();

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;
#else
            dat_size = agc2->output_stream->read();

            if (dat_size <= 0)
            {
                agc2->output_stream->flush();
                continue;
            }

            volk_32f_s32f_convert_16i(output_wav_buffer, (float *)agc2->output_stream->readBuf, 65535 * 0.2, dat_size); //TODO - 65535 is incorrect; use 32767 and fix percent appropriately

            audio_sink->push_samples(output_wav_buffer, dat_size);

            agc2->output_stream->flush();
#endif

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void XFSKBurstDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        lpf1->stop();
        qua->stop();
        agc2->stop();
        lpf2->stop();
        resamplerf->stop();
        rec->stop();
        rec->output_stream->stopReader();
        // agc2->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string XFSKBurstDemodModule::getID()
    {
        return "xfsk_burst_demod";
    }

    std::vector<std::string> XFSKBurstDemodModule::getParameters()
    {
        std::vector<std::string> params = {};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> XFSKBurstDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<XFSKBurstDemodModule>(input_file, output_file_hint, parameters);
    }
}
