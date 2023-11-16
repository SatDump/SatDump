#include "module_meteor_dump_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/codings/differential/nrzm.h"
#include "meteor_rec_deframer.h"
#include "common/simple_deframer.h"

#define BUFFER_SIZE 8192

namespace meteor
{
    METEORDumpDecoderModule::METEORDumpDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                        constellation(1.0, 0.15, demod_constellation_size)
    {
        soft_buffer = new int8_t[BUFFER_SIZE];
        bit_buffer = new uint8_t[BUFFER_SIZE];
        rfrm_buffer = new uint8_t[BUFFER_SIZE];
        rpkt_buffer = new uint8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> METEORDumpDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> METEORDumpDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    METEORDumpDecoderModule::~METEORDumpDecoderModule()
    {
        delete[] soft_buffer;
        delete[] bit_buffer;
        delete[] rfrm_buffer;
        delete[] rpkt_buffer;
    }

    void METEORDumpDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        logger->info("Using input data " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        time_t lastTime = 0;

        int frame_count = 0;

        diff::NRZMDiff nrzm;
        RecorderDeframer recdef;

        def::SimpleDeframer mtvza_deframer(0x38fb456a, 32, 3040, 0, false);

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            for (int i = 0; i < BUFFER_SIZE; i++)
                bit_buffer[i] = soft_buffer[i] > 0;

            nrzm.decode_bits(bit_buffer, BUFFER_SIZE);

            int nfrm = recdef.work(bit_buffer, BUFFER_SIZE, rfrm_buffer);

            int rpktpos = 0;
            for (int i = 0; i < nfrm; i++)
            {
                uint8_t *bytes = &rfrm_buffer[i * 384];
                memcpy(&rpkt_buffer[rpktpos], bytes + 2, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 50, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 98, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 146, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 194, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 242, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 290, 46);
                rpktpos += 46;
                memcpy(&rpkt_buffer[rpktpos], bytes + 338, 46);
                rpktpos += 46;
            }

            for (int i = 0; i < rpktpos; i++)
                rpkt_buffer[i] ^= 0xAA;

            auto deframed_dat = mtvza_deframer.work(rpkt_buffer, rpktpos);
            frame_count += deframed_dat.size();
            for (auto &frm : deframed_dat)
            {
                data_out.write((char *)frm.data(), 380);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            // module_stats["deframer_lock"] = def->getState() == 12;

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = ""; // def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Deframer : " + deframer_state + ", Frames : " + std::to_string(frame_count));
            }
        }

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void METEORDumpDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR Dump Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // Constellation
        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFFER_SIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
#if 0
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (def->getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (def->getState() == 2 || def->getState() == 6)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
            }
#endif
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string METEORDumpDecoderModule::getID()
    {
        return "meteor_dump_decoder";
    }

    std::vector<std::string> METEORDumpDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> METEORDumpDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<METEORDumpDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor