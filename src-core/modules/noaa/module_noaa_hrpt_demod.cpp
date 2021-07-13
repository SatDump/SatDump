#include "module_noaa_hrpt_demod.h"
#include "common/dsp/lib/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    NOAAHRPTDemodModule::NOAAHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                    d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                                    d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                                    constellation(90.0f / 100.0f, 15.0f / 100.0f, demod_constellation_size)
    {
        // Buffers
        bits_buffer = new uint8_t[d_buffer_size * 10];
        snr = 0;
    }

    void NOAAHRPTDemodModule::init()
    {
        float symbolrate = 665400;
        float sps = float(d_samplerate) / symbolrate;

        logger->info("SPS : " + std::to_string(sps));

        // Init DSP blocks
        if (input_data_type == DATA_FILE)
            file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);

        // AGC
        agc = std::make_shared<dsp::AGCBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream, 0.002e-3f, 1.0f, 0.5f / 32768.0f, 65536);

        // PLL
        pll = std::make_shared<dsp::BPSKCarrierPLLBlock>(agc->output_stream, 0.01f, powf(0.01f, 2) / 4.0f, (3.0f * M_PI * 100e3f) / (float)d_samplerate);

        // RRC
        rrc = std::make_shared<dsp::FFFIRBlock>(pll->output_stream, 1, dsp::firgen::root_raised_cosine(1, (float)d_samplerate / 2.0f, symbolrate, 0.5f, 31));

        // Clock Recovery
        rec = std::make_shared<dsp::FFMMClockRecoveryBlock>(rrc->output_stream, sps / 2.0f, powf(0.01, 2) / 4.0f, 0.5f, 0.01f, 100e-6f);

        // Deframer
        def = std::make_shared<NOAADeframer>(std::stoi(d_parameters["deframer_thresold"]));
    }

    std::vector<ModuleDataType> NOAAHRPTDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_DSP_STREAM};
    }

    std::vector<ModuleDataType> NOAAHRPTDemodModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NOAAHRPTDemodModule::~NOAAHRPTDemodModule()
    {
        delete[] bits_buffer;
    }

    void NOAAHRPTDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        //if (input_data_type == DATA_FILE)
        //    data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".raw16", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".raw16");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".raw16");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        if (input_data_type == DATA_FILE)
            file_source->start();
        agc->start();
        pll->start();
        rrc->start();
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

            volk_32f_binary_slicer_8i((int8_t *)bits_buffer, rec->output_stream->readBuf, dat_size);

            rec->output_stream->flush();

            std::vector<uint16_t> frames = def->work(bits_buffer, dat_size);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
                data_out.write((char *)&frames[0], frames.size() * sizeof(uint16_t));

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frame_count / 11090));
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void NOAAHRPTDemodModule::stop()
    {
        // Stop
        if (input_data_type == DATA_FILE)
            file_source->stop();
        agc->stop();
        pll->stop();
        rrc->stop();
        rec->stop();
        rec->output_stream->stopReader();

        data_out.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void NOAAHRPTDemodModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA HRPT Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.pushFloatAndGaussian(rec->output_stream->readBuf, rec->output_stream->getDataSize());
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("SNR (dB) : ");
                ImGui::SameLine();
                ImGui::TextColored(snr > 2 ? snr > 10 ? colorSynced : colorSyncing : colorNosync, UITO_C_STR(snr));

                std::memmove(&snr_history[0], &snr_history[1], (200 - 1) * sizeof(float));
                snr_history[200 - 1] = snr;

                ImGui::PlotLines("", snr_history, IM_ARRAYSIZE(snr_history), 0, "", 0.0f, 25.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frame_count / 11090));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAAHRPTDemodModule::getID()
    {
        return "noaa_hrpt_demod";
    }

    std::vector<std::string> NOAAHRPTDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format", "deframer_thresold"};
    }

    std::shared_ptr<ProcessingModule> NOAAHRPTDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<NOAAHRPTDemodModule>(input_file, output_file_hint, parameters);
    }

    std::vector<uint8_t> NOAAHRPTDemodModule::getBytes(uint8_t *bits, int length)
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
} // namespace noaa