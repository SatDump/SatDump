#include "module_tlm_demod.h"
#include "common/dsp/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace elektro_arktika
{
    TLMDemodModule::TLMDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                          d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                          d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                          constellation(1, 1, demod_constellation_size)
    {
        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 2];
        bits_buffer = new uint8_t[d_buffer_size * 2];
        repacked_buffer = new uint8_t[d_buffer_size * 2];
        real_buffer = new float[d_buffer_size * 2];
        snr = 0;
        peak_snr = 0;
    }

    void TLMDemodModule::init()
    {
        int symbolrate = 5e3;
        float input_sps = (float)d_samplerate / (float)symbolrate;         // Compute input SPS
        resample = input_sps > MAX_SPS;                                    // If SPS is over MAX_SPS, we resample
        float samplerate = resample ? symbolrate * MAX_SPS : d_samplerate; // Get the final samplerate we'll be working with
        float decimation_factor = d_samplerate / samplerate;               // Decimation factor to rescale our input buffer

        if (resample)
            d_buffer_size *= round(decimation_factor);

        float sps = samplerate / (float)symbolrate;

        logger->debug("Input SPS : " + std::to_string(input_sps));
        logger->debug("Resample : " + std::to_string(resample));
        logger->debug("Samplerate : " + std::to_string(samplerate));
        logger->debug("Dec factor : " + std::to_string(decimation_factor));
        logger->debug("Final SPS : " + std::to_string(sps));

        // Init DSP Blocks
        if (input_data_type == DATA_FILE)
            file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);

        // Cleanup things a bit
        std::shared_ptr<dsp::stream<complex_t>> input_data = input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream;

        // Init resampler if required
        if (resample)
            res = std::make_shared<dsp::CCRationalResamplerBlock>(input_data, samplerate, d_samplerate);

        // AGC
        agc = std::make_shared<dsp::AGCBlock>(resample ? res->output_stream : input_data, 0.001f, 1.0f, 1.0f, 65536);

        // Carrier tracking
        cpl = std::make_shared<dsp::PLLCarrierTrackingBlock>(agc->output_stream, 6e-3f, 10e3, -10e3);

        // DC Blocking
        dcb = std::make_shared<dsp::DCBlockerBlock>(cpl->output_stream, 32, true);

        // Frequency shift
        shi = std::make_shared<dsp::FreqShiftBlock>(dcb->output_stream, samplerate, -symbolrate);

        // RRC
        rrc = std::make_shared<dsp::CCFIRBlock>(shi->output_stream, dsp::firdes::root_raised_cosine(1, samplerate, symbolrate, 0.5, 31));

        // Costas
        pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, 0.03f, 2);

        // Clock recovery
        rec = std::make_shared<dsp::CCMMClockRecoveryBlock>(pll->output_stream, sps, 0.625e-3, 0.5f, 0.175, 0.005f);
    }

    std::vector<ModuleDataType> TLMDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_DSP_STREAM};
    }

    std::vector<ModuleDataType> TLMDemodModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    TLMDemodModule::~TLMDemodModule()
    {
        delete[] sym_buffer;
        delete[] bits_buffer;
        delete[] real_buffer;
        delete[] repacked_buffer;
    }

    void TLMDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".cadu");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        if (input_data_type == DATA_FILE)
            file_source->start();
        if (resample)
            res->start();
        agc->start();
        cpl->start();
        dcb->start();
        shi->start();
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
            snr_estimator.update(rec->output_stream->readBuf, dat_size);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            rec->output_stream->flush();

            volk_32fc_deinterleave_real_32f(real_buffer, (lv_32fc_t *)rec->output_stream->readBuf, dat_size);
            volk_32f_binary_slicer_8i((int8_t *)bits_buffer, real_buffer, dat_size);

            int bytes = repacker.work(bits_buffer, dat_size, repacked_buffer);

            std::vector<std::array<uint8_t, CADU_SIZE>> frames = deframer.work(repacked_buffer, bytes);

            for (std::array<uint8_t, CADU_SIZE> cadu : frames)
            {
                if (output_data_type == DATA_FILE)
                    data_out.write((char *)&cadu, CADU_SIZE);
                else
                    output_fifo->write((uint8_t *)&cadu, CADU_SIZE);
            }

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["lock_state"] = deframer.getState();

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

    void TLMDemodModule::stop()
    {
        // Stop
        if (input_data_type == DATA_FILE)
            file_source->stop();
        if (resample)
            res->stop();
        agc->stop();
        cpl->stop();
        dcb->stop();
        shi->stop();
        rrc->stop();
        pll->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void TLMDemodModule::drawUI(bool window)
    {
        ImGui::Begin("ELEKTRO-L / ARKTIKA-M TLM Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.pushComplex(rec->output_stream->readBuf, rec->output_stream->getDataSize());
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            {
                // Show SNR information
                ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
                snr_plot.draw(snr, peak_snr);
            }

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string TLMDemodModule::getID()
    {
        return "elektro_arktika_tlm_demod";
    }

    std::vector<std::string> TLMDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> TLMDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<TLMDemodModule>(input_file, output_file_hint, parameters);
    }
}