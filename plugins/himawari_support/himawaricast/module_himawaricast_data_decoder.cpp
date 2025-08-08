#include "module_himawaricast_data_decoder.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "common/mpeg_ts/fazzt_processor.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/mpeg_ts/ts_header.h"
#include "common/utils.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "libs/bzlib_utils.h"
#include "logger.h"
#include "xrit/processor/xrit_channel_processor_render.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace himawari
{
    namespace himawaricast
    {
        HimawariCastDataDecoderModule::HimawariCastDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
            processor.directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGES";
        }

        HimawariCastDataDecoderModule::~HimawariCastDataDecoderModule() { processor.flush(); }

        void HimawariCastDataDecoderModule::process()
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
            fazzt::FazztProcessor fazzt_processor(1411);

            if (!std::filesystem::exists(directory + "/IMAGES/Unknown"))
                std::filesystem::create_directories(directory + "/IMAGES/Unknown");

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
                    std::vector<std::vector<uint8_t>> frames = ts_demux.demux(mpeg_ts, 1001);

                    for (std::vector<uint8_t> &payload : frames)
                    {
                        payload.erase(payload.begin(), payload.begin() + 40); // Extract the Fazzt frame

                        std::vector<fazzt::FazztFile> files = fazzt_processor.work(payload);

                        for (fazzt::FazztFile &file : files)
                        {
                            size_t buffer_output_size = 50 * 1e6; // 50MB. Should be enough
                            size_t total_output_size = 0;
                            std::vector<uint8_t> out_buffer(buffer_output_size);
                            bool decomp_error = false;
                            uint8_t *out_ptr = out_buffer.data();

                            // Iterate through all compressed blocks... There can be more than one (with, what the heck, each a file signature...)!
                            for (size_t i = 0; i < file.size;)
                            {
                                char *file_c = (char *)file.data.data();
                                if (file_c[i + 0] == 'B' && file_c[i + 1] == 'Z' && file_c[i + 2] == 'h')
                                {
                                    unsigned int local_outsize = buffer_output_size;
                                    unsigned int consumed;

                                    int ret = BZ2_bzBuffToBuffDecompress_M((char *)out_ptr, &local_outsize, (char *)&file.data[i], file.data.size() - i, &consumed, false, 1);
                                    if (ret != BZ_OK && ret != BZ_STREAM_END)
                                    {
                                        decomp_error = true;
                                        logger->error("Failed decompressing Bzip2 data! Error : " + std::to_string(ret));
                                    }

                                    buffer_output_size -= local_outsize;
                                    out_ptr += local_outsize;
                                    total_output_size += local_outsize;
                                    i += consumed;
                                }
                                else
                                    i++; // Just in case...
                            }

                            // Write it all to the file
                            out_buffer.resize(total_output_size);
                            file.data = out_buffer;
                            file.size = total_output_size;
                            out_buffer.clear();

                            // Then carry on with the processing
                            if (decomp_error)
                            {
                                logger->warn("Error decompressing, discarding file...");
                            }
                            else
                            {
                                try
                                {
                                    if (file.name.find("IMG_") != std::string::npos)
                                        file.name = (file.name.substr(0, file.name.size() - 4)) + ".hrit";
                                    else
                                        file.name = (file.name.substr(0, file.name.size() - 4));

                                    if (file.name.find("IMG_") != std::string::npos)
                                    {
                                        satdump::xrit::XRITFile lfile;
                                        lfile.lrit_data = file.data;
                                        lfile.parseHeaders();

                                        satdump::xrit::PrimaryHeader primary_header = lfile.getHeader<satdump::xrit::PrimaryHeader>();

                                        if (lfile.lrit_data.size() != (primary_header.data_field_length / 8) + primary_header.total_header_length)
                                            continue;

                                        // Check if this has a filename
                                        logger->info("New xRIT file : " + lfile.filename);

                                        satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(lfile);

                                        // Check if this is image data, and if so also write it as an image
                                        if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
                                        {
                                            processor.push(finfo, lfile);
                                        }
                                        else
                                        {
                                            logger->debug("Saving " + file.name + " size " + std::to_string(file.size));

                                            if (!std::filesystem::exists(directory + "/LRIT"))
                                                std::filesystem::create_directories(directory + "/LRIT");

                                            std::ofstream output_himawari_file(directory + "/LRIT/" + file.name);
                                            output_himawari_file.write((char *)file.data.data(), file.data.size());
                                            output_himawari_file.close();
                                        }
                                    }
                                    else
                                    {
                                        logger->debug("Saving " + file.name + " size " + std::to_string(file.size));

                                        if (!std::filesystem::exists(directory + "/ADD"))
                                            std::filesystem::create_directories(directory + "/ADD");

                                        std::ofstream output_himawari_file(directory + "/ADD/" + file.name);
                                        output_himawari_file.write((char *)file.data.data(), file.data.size());
                                        output_himawari_file.close();
                                    }
                                }
                                catch (std::exception &e)
                                {
                                    logger->error("Error processing HimawariCast file %s", e.what());
                                }
                            }
                        }
                    }
                }
            }

            cleanup();
        }

        void HimawariCastDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("HimawariCast Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            satdump::xrit::renderAllTabsFromProcessors({&processor});

            drawProgressBar();

            ImGui::End();
        }

        std::string HimawariCastDataDecoderModule::getID() { return "himawaricast_data_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> HimawariCastDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<HimawariCastDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace himawaricast
} // namespace himawari