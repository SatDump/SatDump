#include "module_terra_db_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace terra
{
    TerraDBDemodModule::TerraDBDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : demod::BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 2];

        // Window Name in the UI
        name = "Terra DB Demodulator";

        // Show freq
        show_freq = true;
    }

    void TerraDBDemodModule::init()
    {
        BaseDemodModule::init();
        rrc = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate * 2, 0.5f, 31));
        pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, 0.004, 2);
        rec = std::make_shared<dsp::MMClockRecoveryBlock<complex_t>>(pll->output_stream, ((float)final_samplerate / (float)d_symbolrate) / 2.0f, pow(0.001, 2) / 4.0, 0.5f, 0.001, 0.0001f);
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

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".soft");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : {:d}", d_buffer_size);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        rrc->start();
        pll->start();
        rec->start();

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
            {
                rec->output_stream->flush();
                continue;
            }

            // Push into constellation
            constellation.pushComplexScaled(rec->output_stream->readBuf, dat_size, 0.5);

            // Estimate SNR
            snr_estimator.update(rec->output_stream->readBuf, dat_size);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            // Update freq
            display_freq = dsp::rad_to_hz(pll->getFreq(), final_samplerate);

            for (int i = 0; i < dat_size; i++)
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i].real * 50);

            rec->output_stream->flush();

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, dat_size);
            else
                output_fifo->write((uint8_t *)sym_buffer, dat_size);

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

    void TerraDBDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();

        rrc->stop();
        pll->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string TerraDBDemodModule::getID()
    {
        return "terra_db_demod";
    }

    std::vector<std::string> TerraDBDemodModule::getParameters()
    {
        return BaseDemodModule::getParameters();
    }

    std::shared_ptr<ProcessingModule> TerraDBDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TerraDBDemodModule>(input_file, output_file_hint, parameters);
    }
}
