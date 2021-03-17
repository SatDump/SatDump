#include "module_meteor_hrpt_demod.h"
#include "modules/common/dsp/lib/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    METEORHRPTDemodModule::METEORHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                        d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                                        d_buffer_size(std::stoi(parameters["buffer_size"]))
    {
        // Init DSP blocks
        file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(parameters["baseband_format"]), d_buffer_size);
        agc = std::make_shared<dsp::AGCBlock>(file_source->output_stream, 0.0038e-3f, 1.0f, 0.5f / 32768.0f, 65536);
        rrc = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, 1, dsp::firgen::root_raised_cosine(1, d_samplerate, 665400.0f * 2.2f, 0.5f, 31));
        pll = std::make_shared<dsp::BPSKCarrierPLLBlock>(rrc->output_stream, 0.030f, powf(0.030f, 2) / 4.0f, 0.5f);
        mov = std::make_shared<dsp::FFMovingAverageBlock>(pll->output_stream, round(((float)d_samplerate / (float)665400) / 2.0f), 1.0 / round(((float)d_samplerate / (float)665400) / 2.0f), d_buffer_size, 1);
        rec = std::make_shared<dsp::FFMMClockRecoveryBlock>(mov->output_stream, ((float)d_samplerate / (float)665400) / 2.0f, powf(40e-3, 2) / 4.0f, 1.0f, 40e-3, 0.01f);

        // Buffers
        bits_buffer = new uint8_t[d_buffer_size * 10];
    }

    std::vector<ModuleDataType> METEORHRPTDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> METEORHRPTDemodModule::getOutputTypes()
    {
        return {DATA_FILE};
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

        //if (input_data_type == DATA_FILE)
        //    data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".dem", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".dem");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".dem");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        file_source->start();
        agc->start();
        rrc->start();
        pll->start();
        mov->start();
        rec->start();

        int dat_size = 0;
        while (/*input_data_type == DATA_STREAM ? input_active.load() : */ !file_source->eof())
        {
            dat_size = rec->output_stream->read(); //->read(rec_buffer2, d_buffer_size);

            if (dat_size <= 0)
                continue;

            volk_32f_binary_slicer_8i((int8_t *)bits_buffer, rec->output_stream->readBuf, dat_size);

            rec->output_stream->flush();

            std::vector<uint8_t> bytes = getBytes(bits_buffer, dat_size);

            data_out.write((char *)&bytes[0], bytes.size());

            progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        logger->info("Demodulation finished");

        // Stop
        file_source->stop();
        agc->stop();
        rrc->stop();
        pll->stop();
        mov->stop();
        rec->stop();

        data_out.close();
    }

    void METEORHRPTDemodModule::drawUI()
    {
        ImGui::Begin("METEOR HRPT Demodulator", NULL, NOWINDOW_FLAGS);

        // Constellation
        {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                     ImColor::HSV(0, 0, 0));

            for (int i = 0; i < 2048; i++)
            {
                draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + rec->output_stream->readBuf[i] * 90) % 200,
                                                  ImGui::GetCursorScreenPos().y + (int)(100 + rng.gasdev() * 15) % 200),
                                           2,
                                           ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
            }

            ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
        }

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

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