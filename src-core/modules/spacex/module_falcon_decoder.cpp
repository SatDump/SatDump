#include "module_falcon_decoder.h"
#include "logger.h"
#include "modules/common/sathelper/derandomizer.h"
#include "imgui/imgui.h"
#include "demuxer.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace spacex
{
    FalconDecoderModule::FalconDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    FalconDecoderModule::~FalconDecoderModule()
    {
    }

    void FalconDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);

        logger->info("Using input CADUs " + d_input_file);

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        std::ofstream video_out = std::ofstream(directory + "/camera.mxf", std::ios::binary);
        d_output_files.push_back(directory + "/camera.mxf");
        logger->info("Decoding to " + directory + "/camera.mxf");
        logger->warn("This MXF (MPEG) video stream may need to be converted to be usable. GStreamer is recommended as it gives significantly better results than most other software!");
        logger->warn("gst-launch-1.0 filesrc location=\"camera.mxf\" ! decodebin ! videoconvert ! avimux name=mux ! filesink location=camera.avi");

        std::ofstream gps_debug_out = std::ofstream(directory + "/gps_debug.txt");
        d_output_files.push_back(directory + "/gps_debug.txt");
        logger->info("Decoding to " + directory + "/gps_debug.txt");

        std::ofstream sfc1a_debug = std::ofstream(directory + "/sfc1a_debug.txt");
        d_output_files.push_back(directory + "/sfc1a_debug.txt");
        logger->info("Decoding to " + directory + "/sfc1a_debug.txt");

        std::ofstream sftmm1a_debug = std::ofstream(directory + "/sftmm1a_debug.txt");
        d_output_files.push_back(directory + "/sftmm1a_debug.txt");
        logger->info("Decoding to " + directory + "/sftmm1a_debug.txt");

        std::ofstream sftmm1b_debug = std::ofstream(directory + "/sftmm1b_debug.txt");
        d_output_files.push_back(directory + "/sftmm1b_debug.txt");
        logger->info("Decoding to " + directory + "/sftmm1b_debug.txt");

        std::ofstream sftmm1c_debug = std::ofstream(directory + "/sftmm1c_debug.txt");
        d_output_files.push_back(directory + "/sftmm1c_debug.txt");
        logger->info("Decoding to " + directory + "/sftmm1c_debug.txt");

        //  data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        //  d_output_files.push_back(d_output_file_hint + ".frm");
        //  logger->info("Decoding to " + d_output_file_hint + ".frm");

        time_t lastTime = 0;

        uint8_t cadu[1279];
        Demuxer demux;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)cadu, 1279);

            std::vector<SpaceXPacket> frames = demux.work(cadu);

            for (SpaceXPacket pkt : frames)
            {
                uint64_t marker = ((uint64_t)pkt.payload[2]) << 56 | ((uint64_t)pkt.payload[3]) << 48 |
                                  ((uint64_t)pkt.payload[4]) << 40 | ((uint64_t)pkt.payload[5]) << 32 |
                                  ((uint64_t)pkt.payload[6]) << 24 | ((uint64_t)pkt.payload[7]) << 16 |
                                  ((uint64_t)pkt.payload[8]) << 8 | ((uint64_t)pkt.payload[9]);

                if (marker == 0x01123201042E1403 && pkt.payload.size() >= 965.0)
                {
                    video_out.write((char *)&pkt.payload[25], 965.0 - 25);
                }
                else if (marker == 0x0117FE0800320303 || marker == 0x0112FA0800320303)
                {
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    gps_debug_out << std::string((char *)&pkt.payload[25]) << std::endl;
                }
                else if (marker == 0x0112220100620303)
                {
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sfc1a_debug << std::string((char *)&pkt.payload[25]) << std::endl;
                }
                else if (marker == 0x0112520100620303)
                {
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sftmm1a_debug << std::string((char *)&pkt.payload[25]) << std::endl;
                }
                else if (marker == 0x0112520100620303)
                {
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sftmm1b_debug << std::string((char *)&pkt.payload[25]) << std::endl;
                }
                else if (marker == 0x0112720100620303)
                {
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sftmm1c_debug << std::string((char *)&pkt.payload[25]) << std::endl;
                }

                // if (marker == 0x0012FA08D0480108)
                // {
                // logger->info(pkt.payload.size());

                //     data_out.put(0x1a);
                //     data_out.put(0xcf);
                //      data_out.put(0xfc);
                //      data_out.put(0x1d);
                //      data_out.write((char *)pkt.payload.data(), pkt.payload.size());
                // }
            }
            progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f));
            }
        }

        video_out.close();
        gps_debug_out.close();
        sfc1a_debug.close();
        sftmm1a_debug.close();
        sftmm1b_debug.close();
        sftmm1c_debug.close();

        //data_out.close();
        data_in.close();
    }

    void FalconDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Falcon 9 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS );

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string FalconDecoderModule::getID()
    {
        return "falcon_decoder";
    }

    std::vector<std::string> FalconDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> FalconDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<FalconDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace falcon