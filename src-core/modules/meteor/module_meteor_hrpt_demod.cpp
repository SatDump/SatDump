#include "module_meteor_hrpt_demod.h"
#include "common/dsp/lib/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    METEORHRPTDemodModule::METEORHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                        d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                                        d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                                        constellation(90.0f / 100.0f, 15.0f / 100.0f, demod_constellation_size)
    {
        // Buffers
        bits_buffer = new uint8_t[d_buffer_size * 10];
        snr = 0;
        peak_snr = 0;
    }

    void METEORHRPTDemodModule::init()
    {
        float symbolrate = 665400;
        float sps = float(d_samplerate) / symbolrate;

        logger->info("SPS : " + std::to_string(sps));

        // Init DSP blocks
        if (input_data_type == DATA_FILE)
            file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);

        // AGC
        agc = std::make_shared<dsp::AGCBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream, 0.0038e-3f, 1.0f, 0.5f / 32768.0f, 65536);

        // RRC
        rrc = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, 1, dsp::firgen::root_raised_cosine(1, d_samplerate, symbolrate * 2.2f, 0.5f, 31));

        // PLL
        pll = std::make_shared<dsp::BPSKCarrierPLLBlock>(rrc->output_stream, 0.030f, powf(0.030f, 2) / 4.0f, 0.5f);

        // Moving AVG
        mov = std::make_shared<dsp::FFMovingAverageBlock>(pll->output_stream, round(sps / 2.0f), 1.0 / round(sps / 2.0f), d_buffer_size, 1);

        // Clock recovery
        rec = std::make_shared<dsp::FFMMClockRecoveryBlock>(mov->output_stream, sps / 2.0f, powf(40e-3, 2) / 4.0f, 1.0f, 40e-3, 0.001f);
    }

    std::vector<ModuleDataType> METEORHRPTDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_DSP_STREAM};
    }

    std::vector<ModuleDataType> METEORHRPTDemodModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    METEORHRPTDemodModule::~METEORHRPTDemodModule()
    {
        delete[] bits_buffer;
    }

    void METEORHRPTDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".dem", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".dem");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".dem");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        if (input_data_type == DATA_FILE)
            file_source->start();
        agc->start();
        rrc->start();
        pll->start();
        mov->start();
        rec->start();

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
                continue;

            // Estimate SNR, only on part of the samples to limit CPU usage
            snr_estimator.update((std::complex<float> *)rec->output_stream->readBuf, dat_size / 100);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            volk_32f_binary_slicer_8i((int8_t *)bits_buffer, rec->output_stream->readBuf, dat_size);

            rec->output_stream->flush();

            std::vector<uint8_t> bytes = getBytes(bits_buffer, dat_size);

            if (output_data_type == DATA_FILE)
                data_out.write((char *)&bytes[0], bytes.size());
            else
                output_fifo->write((uint8_t *)&bytes[0], bytes.size());

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void METEORHRPTDemodModule::stop()
    {
        // Stop
        if (input_data_type == DATA_FILE)
            file_source->stop();
        agc->stop();
        rrc->stop();
        pll->stop();
        mov->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void METEORHRPTDemodModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR HRPT Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

        // Constellation
        constellation.pushFloatAndGaussian(rec->output_stream->readBuf, rec->output_stream->getDataSize());
        constellation.draw();

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

    std::string METEORHRPTDemodModule::getID()
    {
        return "meteor_hrpt_demod";
    }

    std::vector<std::string> METEORHRPTDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> METEORHRPTDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<METEORHRPTDemodModule>(input_file, output_file_hint, parameters);
    }

    std::vector<uint8_t> METEORHRPTDemodModule::getBytes(uint8_t *bits, int length)
    {
        std::vector<uint8_t> bytesToRet;
        for (int ii = 0; ii < length; ii++)
        {
            byteToWrite = (byteToWrite << 1) | bits[ii];
            inByteToWrite++;

            if (inByteToWrite == 8)
            {
                bytesToRet.push_back(byteToWrite);
                inByteToWrite = 0;
            }
        }

        return bytesToRet;
    }
} // namespace meteor
