#include "module_geonetcast_decoder.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "common/mpeg_ts/fazzt_processor.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/mpeg_ts/ts_header.h"
#include "common/utils.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "libs/ctpl/ctpl_stl.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "goes_abi.h"

#include "utils/thread_priority.h"

namespace geonetcast
{
    GeoNetCastDecoderModule::GeoNetCastDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    GeoNetCastDecoderModule::~GeoNetCastDecoderModule() {}

    void GeoNetCastDecoderModule::process()
    {
        directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        if (!std::filesystem::exists(directory))
            std::filesystem::create_directory(directory);

        bool ts_input = d_parameters["ts_input"].get<bool>();

        // BBFrame stuff
        uint8_t bb_frame[38688];
        dvbs2::BBFrameTSParser ts_extractor(38688);

        // TS Stuff
        int ts_cnt = 1;
        uint8_t mpeg_ts_all[188 * 1000];
        mpeg_ts::TSDemux ts_demux;
        fazzt::FazztProcessor fazzt_processor(1408);

        ctpl::thread_pool saving_pool(16);

        while (should_run())
        {
            if (ts_input)
            {
                // Read buffer
                read_data((uint8_t *)mpeg_ts_all, 188);
                ts_cnt = 1;
            }
            else
            {
                // Read buffer
                read_data((uint8_t *)bb_frame, 38688 / 8);

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

                    for (fazzt::FazztFile file : files)
                    {
                        bool has_all_parts = true;
                        for (auto p : file.has_parts)
                            if (!p)
                                has_all_parts = false;

                        file.name = file.name.substr(0, file.name.length() - 45);

                        auto function = [file, has_all_parts, this](int)
                        {
                            std::string filesize_str = "";
                            if (file.size >= 1e3)
                                filesize_str = std::to_string(file.size / 1e3) + "kB";
                            if (file.size >= 1e6)
                                filesize_str = std::to_string(file.size / 1e6) + "MB";
                            if (file.size >= 1e9)
                                filesize_str = std::to_string(file.size / 1e9) + "GB";

                            if (has_all_parts)
                            {
                                if (!std::filesystem::exists(directory + "/COMPLETE"))
                                    std::filesystem::create_directories(directory + "/COMPLETE");

                                if (!std::filesystem::exists(directory + "/PROCESSED"))
                                    std::filesystem::create_directories(directory + "/PROCESSED");

                                // Try to process as GOES ABI
                                if (this->d_parameters["process_goes_abi"].get<bool>())
                                {
#ifdef ENABLE_HDF5_PARSING
                                    int mode = 0;
                                    int channel = 0;
                                    int satellite = 0;
                                    uint64_t tstart = 0;
                                    uint64_t tend = 0;
                                    uint64_t tfile = 0;
                                    uint64_t tidk = 0;

                                    if (sscanf(file.name.c_str(), "OR_ABI-L2-CMIPF-M%1dC%2d_G%2d_s%lu_e%lu_c%lu-%lu_0.nc", &mode, &channel, &satellite, &tstart, &tend, &tfile, &tidk) == 7)
                                    {
                                        logger->info("Mode %d Channel %d Satellite %d", mode, channel, satellite);

                                        int bit_depth = 12;

                                        if (channel == 7)
                                            bit_depth = 14;

                                        image::Image final_image = parse_goesr_abi_netcdf_fulldisk_CMI(file.data, bit_depth);

                                        image::save_img(final_image, directory + "/PROCESSED/" + file.name);
                                        // return;
                                    }
#endif
                                }

                                logger->debug("Saving complete " + file.name + " size " + filesize_str);

                                std::ofstream output_geonetcast_file(directory + "/COMPLETE/" + file.name);
                                output_geonetcast_file.write((char *)file.data.data(), file.data.size());
                                output_geonetcast_file.close();
                            }
                            else
                            {
                                if (!std::filesystem::exists(directory + "/INCOMPLETE"))
                                    std::filesystem::create_directories(directory + "/INCOMPLETE");

                                logger->trace("Saving incomplete " + file.name + " size " + filesize_str);

                                std::ofstream output_geonetcast_file(directory + "/INCOMPLETE/" + file.name);
                                output_geonetcast_file.write((char *)file.data.data(), file.data.size());
                                output_geonetcast_file.close();
                            }
                        };

                        saving_pool.push(function);
                    }
                }
            }
        }

        cleanup();
    }

    void GeoNetCastDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("GeoNetCast Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        drawProgressBar();

        ImGui::End();
    }

    std::string GeoNetCastDecoderModule::getID() { return "geonetcast_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> GeoNetCastDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GeoNetCastDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace geonetcast