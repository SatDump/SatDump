#include "module_geonetcast_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "common/mpeg_ts/ts_header.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/mpeg_ts/fazzt_processor.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"

namespace geonetcast
{
    GeoNetCastDecoderModule::GeoNetCastDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    std::vector<ModuleDataType> GeoNetCastDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> GeoNetCastDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    GeoNetCastDecoderModule::~GeoNetCastDecoderModule()
    {
    }

    void GeoNetCastDecoderModule::process()
    {
        std::ifstream data_in;

        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        if (!std::filesystem::exists(directory))
            std::filesystem::create_directory(directory);

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        time_t lastTime = 0;

        bool ts_input = d_parameters["ts_input"].get<bool>();

        // BBFrame stuff
        uint8_t bb_frame[38688];
        dvbs2::BBFrameTSParser ts_extractor(38688);

        // TS Stuff
        int ts_cnt = 1;
        uint8_t mpeg_ts_all[188 * 1000];
        mpeg_ts::TSDemux ts_demux;
        fazzt::FazztProcessor fazzt_processor(1408);

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            if (ts_input)
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)mpeg_ts_all, 188);
                else
                    input_fifo->read((uint8_t *)mpeg_ts_all, 188);
                ts_cnt = 1;
            }
            else
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)bb_frame, 38688 / 8);
                else
                    input_fifo->read((uint8_t *)bb_frame, 38688 / 8);
                ts_cnt = ts_extractor.work(bb_frame, 1, mpeg_ts_all, 188 * 1000);
            }

            for (int tsc = 0; tsc < ts_cnt; tsc++)
            {
                uint8_t *mpeg_ts = &mpeg_ts_all[tsc * 188];
                std::vector<std::vector<uint8_t>> frames = ts_demux.demux(mpeg_ts, 4201);

                for (std::vector<uint8_t> &payload : frames)
                {
                    payload.erase(payload.begin(), payload.begin() + 40); // Extract the Fazzt frame

                    std::vector<fazzt::FazztFile> files = fazzt_processor.work(payload);

                    for (fazzt::FazztFile &file : files)
                    {
                        logger->debug("Saving " + file.name + " size " + std::to_string(file.size));

                        std::ofstream output_himawari_file(directory + "/" + file.name);
                        output_himawari_file.write((char *)file.data.data(), file.data.size());
                        output_himawari_file.close();
                    }
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        data_in.close();
    }

    void GeoNetCastDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("GeoNetCast Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string GeoNetCastDecoderModule::getID()
    {
        return "geonetcast_decoder";
    }

    std::vector<std::string> GeoNetCastDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> GeoNetCastDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GeoNetCastDecoderModule>(input_file, output_file_hint, parameters);
    }
}