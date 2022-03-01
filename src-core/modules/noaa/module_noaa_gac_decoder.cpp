#include "module_noaa_gac_decoder.h"
#include "common/dsp/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include <filesystem>

// Return filesize
size_t getFilesize(std::string filepath);

#define BUFSIZE 8192

namespace noaa
{
#include "gac_pn.h"

    NOAAGACDecoderModule::NOAAGACDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  constellation(1.0, 0.15, demod_constellation_size),
                                                                                                                                  backward(parameters["backward"].get<bool>())
    {
        // Buffers
        soft_buffer = new int8_t[BUFSIZE];
        deframer = std::make_shared<deframing::BPSK_CCSDS_Deframer>(33270, backward ? 0x33c3e4a6 : 0xA116FD71);
        deframer->CADU_PADDING = 33270 % 8;
    }

    std::vector<ModuleDataType> NOAAGACDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NOAAGACDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NOAAGACDecoderModule::~NOAAGACDecoderModule()
    {
        delete[] soft_buffer;
    }

    void NOAAGACDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        if (backward)
            data_out = std::ofstream(d_output_file_hint + ".frm.tmp", std::ios::binary);
        else
            data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        time_t lastTime = 0;

        uint8_t bits_out[BUFSIZE];
        uint8_t frame_buffer[4159 * 10];

        // Init PN Sequence as bytes
        uint8_t gac_pn[4159];
        memset(gac_pn, 0, 4159);

        for (int i = 0; i < 4159 * 8 - 60; i++)
        {
            int real_bit = i + 60;
            gac_pn[real_bit / 8] = gac_pn[real_bit / 8] << 1 | gac_rand_bits[i % 1023];
        }

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFSIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFSIZE);

            for (int i = 0; i < BUFSIZE; i++)
                bits_out[i] = soft_buffer[i] > 0;

            int frames = deframer->work(bits_out, BUFSIZE, frame_buffer);

            // Count frames
            frame_count += frames;

            for (int i = 0; i < frames; i++)
            {
                uint8_t *frm = &frame_buffer[i * 4159];

                if (backward) // Backward tape playback
                {
                    uint8_t invert_buf[4159];
                    memcpy(invert_buf, frm, 4159);
                    memset(frm, 0, 4159);

                    for (int ii = 0; ii < 33270; ii++)
                    {
                        int inv = (33270 - 1) - ii;
                        frm[ii / 8] = frm[ii / 8] << 1 | ((invert_buf[inv / 8] >> (7 - (inv % 8))) & 1);
                    }
                }

                for (int i = 0; i < 4159; i++)
                    frm[i] ^= gac_pn[i];

                // Write it out
                data_out.write((char *)frm, 4159);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frame_count / 11090));
            }
        }

        if (backward)
        {
            logger->info("This was a backward playback, reversing frame order...");
            std::ifstream inversed_file(d_output_file_hint + ".frm.tmp", std::ios::binary);
            data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);

            filesize = getFilesize(d_output_file_hint + ".frm.tmp");
            for (int64_t fpos = filesize - 4159; fpos >= 0; fpos -= 4159)
            {
                inversed_file.seekg(fpos);
                inversed_file.read((char *)frame_buffer, 4159);
                data_out.write((char *)frame_buffer, 4159);
                progress = filesize - fpos;
                logger->info(fpos);
            }

            inversed_file.close();
            data_out.close();

            if (std::filesystem::exists(d_output_file_hint + ".frm.tmp"))
                std::filesystem::remove(d_output_file_hint + ".frm.tmp");
            logger->info("Done!");
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            data_in.close();
        data_out.close();
    }

    void NOAAGACDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA GAC Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        // Constellation
        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFSIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer->getState() == deframer->STATE_NOSYNC)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (deframer->getState() == deframer->STATE_SYNCING)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAAGACDecoderModule::getID()
    {
        return "noaa_gac_decoder";
    }

    std::vector<std::string> NOAAGACDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> NOAAGACDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAAGACDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
