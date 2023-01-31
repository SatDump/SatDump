#include "module_noaa_apt_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "common/dsp/io/wav_writer.h"

namespace noaa_apt
{
    NOAAAPTDemodModule::NOAAAPTDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Parse params
        // if (parameters.count("demodulator_bw") > 0)
        //     d_demodulator_bandwidth = parameters["demodulator_bw"].get<long>();
        // else
        //     throw std::runtime_error("Demodulator BW parameter must be present!");

        name = "NOAA APT Demodulator (FM)";
        show_freq = false;

        constellation.d_hscale = 1.0; // 80.0 / 100.0;
        constellation.d_vscale = 0.5; // 20.0 / 100.0;

        MIN_SPS = 1;
        MAX_SPS = 1000.0;
    }

    void NOAAAPTDemodModule::init()
    {
        BaseDemodModule::init();

        // Resampler to BW
        res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, d_symbolrate, final_samplerate);

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(res->output_stream, dsp::hz_to_rad(d_symbolrate / 2, d_symbolrate));
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

        if (output_data_type == DATA_FILE)
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
        qua->start();

        // Buffers to wav
        int16_t *output_wav_buffer = new int16_t[d_buffer_size * 100];
        int final_data_size = 0;
        dsp::WavWriter wave_writer(data_out);
        if (output_data_type == DATA_FILE)
            wave_writer.write_header(d_symbolrate, 1);

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
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

            volk_32f_s32f_convert_16i(output_wav_buffer, (float *)qua->output_stream->readBuf, 65535, dat_size);

            if (output_data_type == DATA_FILE)
            {
                data_out.write((char *)output_wav_buffer, dat_size * sizeof(int16_t));
                final_data_size += dat_size * sizeof(int16_t);
            }
            else
            {
                output_fifo->write((uint8_t *)output_wav_buffer, dat_size * sizeof(int16_t));
            }

            qua->output_stream->flush();

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        // Finish up WAV
        if (output_data_type == DATA_FILE)
            wave_writer.finish_header(final_data_size);
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
        qua->stop();
        qua->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
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