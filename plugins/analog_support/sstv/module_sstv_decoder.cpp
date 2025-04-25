#include "module_sstv_decoder.h"
#include "common/utils.h"
#include "common/wav.h"
#include "core/exception.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "resources.h"

namespace sstv
{
    SSTVDecoderModule::SSTVDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (parameters.count("audio_samplerate") > 0)
            d_audio_samplerate = parameters["audio_samplerate"].get<long>();
        else
            throw satdump_exception("Audio samplerate parameter must be present!");
    }

    SSTVDecoderModule::~SSTVDecoderModule() {}

    void SSTVDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        std::string main_dir = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        bool is_stereo = false;

        std::ifstream data_in;
        if (input_data_type == DATA_FILE)
        {
            wav::WavHeader hdr = wav::parseHeaderFromFileWav(d_input_file);
            if (!wav::isValidWav(hdr))
                throw satdump_exception("File is not WAV!");
            if (hdr.bits_per_sample != 16)
                throw satdump_exception("Only 16-bits WAV are supported!");
            d_audio_samplerate = hdr.samplerate;
            is_stereo = hdr.channel_cnt == 2;
            if (is_stereo)
                logger->info("File is stereo. Ignoring second channel!");

            wav::FileMetadata md = wav::tryParseFilenameMetadata(d_input_file, true);
            if (md.timestamp != 0 && (d_parameters.contains("start_timestamp") ? d_parameters["start_timestamp"] == -1 : 1))
            {
                d_parameters["start_timestamp"] = md.timestamp;
                logger->trace("Has timestamp %d", md.timestamp);
            }
            else
            {
                logger->warn("Couldn't automatically determine the timestamp, in case of unexpected results, please verify you have specified the correct timestamp manually");
            }

            data_in = std::ifstream(d_input_file, std::ios::binary);
        }

        logger->info("Using input wav " + d_input_file);

        // TODOREWORK
    }

    void SSTVDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("SSTV Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // ImGui::BeginGroup();
        // {
        //     if (textureID == 0)
        //     {
        //         textureID = makeImageTexture();
        //         textureBuffer = new uint32_t[262144]; // 512x512
        //         memset(textureBuffer, 0, sizeof(uint32_t) * 262144);
        //         has_to_update = true;
        //     }

        //     if (has_to_update)
        //     {
        //         updateImageTexture(textureID, textureBuffer, 512, 512);
        //         has_to_update = false;
        //     }

        //     ImGui::Image((void *)(intptr_t)textureID, {200 * ui_scale, 200 * ui_scale});
        // }
        // ImGui::EndGroup();

        // ImGui::SameLine();

        // ImGui::BeginGroup();
        // {
        //     ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
        //     drawStatus(apt_status);
        // }
        // ImGui::EndGroup();

        if (input_data_type == DATA_FILE)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string SSTVDecoderModule::getID() { return "sstv_decoder"; }

    std::vector<std::string> SSTVDecoderModule::getParameters() { return {}; }

    std::shared_ptr<ProcessingModule> SSTVDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SSTVDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace sstv