#include "module_falcon_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "demuxer.h"

#define FALCON_CADU_FRAME_SIZE 0x4FF // 1279 bytes

#define STREAM_HEADER_SKIP 0x19 // 25 bytes

#define MAGIC_MARKER_VIDEO_STREAM 0x01123201042E1403
#define VIDEO_STREAM_MIN_DATA_SIZE 0x3C5 // 965 bytes

#define MAGIC_MARKER0_GPS_DEBUG 0x0117FE0800320303
#define MAGIC_MARKER1_GPS_DEBUG 0x0112FA0800320303

#define MAGIC_MARKER_SFC1A_DEBUG 0x0112220100620303
#define MAGIC_MARKER_SFTMM1A_DEBUG 0x0112520100620303
#define MAGIC_MARKER_SFTMM1C_DEBUG 0x0112720100620303

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace spacex
{
    FalconDecoderModule::FalconDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
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

#ifdef USE_VIDEO_ENCODER
        this->videoStreamEnc = std::unique_ptr<FalconVideoEncoder>(new FalconVideoEncoder(directory));
#else
        std::ofstream video_out = std::ofstream(directory + "/camera.mxf", std::ios::binary);
        d_output_files.push_back(directory + "/camera.mxf");
        logger->info("Decoding to " + directory + "/camera.mxf");
        logger->warn("This MXF (MPEG) video stream may need to be converted to be usable. GStreamer is recommended as it gives significantly better results than most other software!");
        logger->warn("gst-launch-1.0 filesrc location=\"camera.mxf\" ! decodebin ! videoconvert ! avimux name=mux ! filesink location=camera.avi");
#endif

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

        uint8_t cadu[FALCON_CADU_FRAME_SIZE];
        Demuxer demux;

        std::string gps_raw_txt, gps_tmp;
#ifdef USE_VIDEO_ENCODER
        uint64_t gps_date_num;
#endif

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)cadu, FALCON_CADU_FRAME_SIZE);

            std::vector<SpaceXPacket> frames = demux.work(cadu);

            for (SpaceXPacket pkt : frames)
            {
                uint64_t marker = ((uint64_t)pkt.payload[2]) << 56 | ((uint64_t)pkt.payload[3]) << 48 |
                                  ((uint64_t)pkt.payload[4]) << 40 | ((uint64_t)pkt.payload[5]) << 32 |
                                  ((uint64_t)pkt.payload[6]) << 24 | ((uint64_t)pkt.payload[7]) << 16 |
                                  ((uint64_t)pkt.payload[8]) << 8 | ((uint64_t)pkt.payload[9]);

                size_t payload_size = pkt.payload.size();

                /* Use switch:case for the faster processing */
                switch (marker)
                {
                case MAGIC_MARKER_VIDEO_STREAM:
                    if (payload_size >= VIDEO_STREAM_MIN_DATA_SIZE)
                    {
                        const uint8_t *vframe = static_cast<const uint8_t *>(&pkt.payload[STREAM_HEADER_SKIP]);
                        size_t vframe_size = VIDEO_STREAM_MIN_DATA_SIZE - STREAM_HEADER_SKIP;

#ifdef USE_VIDEO_ENCODER
                        std::vector<uint8_t>::const_iterator vframe_start = pkt.payload.begin() + STREAM_HEADER_SKIP;
                        std::vector<uint8_t>::const_iterator vframe_end = vframe_start + vframe_size;

                        videoStreamEnc->feedData(vframe_start, vframe_end);
#else
                        video_out.write(reinterpret_cast<const char *>(vframe), vframe_size);
#endif
                    }
                    break;

                case MAGIC_MARKER0_GPS_DEBUG:
                case MAGIC_MARKER1_GPS_DEBUG:
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;

                    gps_raw_txt = std::string((char *)&pkt.payload[STREAM_HEADER_SKIP]);

#ifdef USE_VIDEO_ENCODER
                    if (gps_raw_txt.length() >= 85)
                    {
                        if (gps_raw_txt[19] == '>')
                        {
                            gps_tmp = gps_raw_txt.substr(0, 18);

                            gps_date_num = atol(gps_tmp.c_str());

                            gps_tmp = "GPS timestamp: " + gps_tmp + "\nMessage: " + gps_raw_txt.substr(20, gps_raw_txt.size() - 20);

                            videoStreamEnc->pushTelemetryText(gps_tmp);
                        }
                    }
#endif
                    gps_debug_out << gps_raw_txt << std::endl;
                    break;

                case MAGIC_MARKER_SFC1A_DEBUG:
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sfc1a_debug << std::string((char *)&pkt.payload[STREAM_HEADER_SKIP]) << std::endl;
                    break;

                case MAGIC_MARKER_SFTMM1A_DEBUG:
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sftmm1a_debug << std::string((char *)&pkt.payload[STREAM_HEADER_SKIP]) << std::endl;
                    break;

                case MAGIC_MARKER_SFTMM1C_DEBUG:
                    pkt.payload[pkt.payload.size() - 1] = 0;
                    pkt.payload[pkt.payload.size() - 2] = 0;
                    sftmm1c_debug << std::string((char *)&pkt.payload[STREAM_HEADER_SKIP]) << std::endl;
                    break;

#if 0
                    case 0x0012FA08D0480108:
                        // logger->info(pkt.payload.size());

                        //     data_out.put(0x1a);
                        //     data_out.put(0xcf);
                        //      data_out.put(0xfc);
                        //      data_out.put(0x1d);
                        //      data_out.write((char *)pkt.payload.data(), pkt.payload.size());
#endif

                default:
                    break;
                };
            }

            progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0));
            }
        }

#ifdef USE_VIDEO_ENCODER
        videoStreamEnc->streamFinished();
#else
        video_out.close();
#endif
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
        ImGui::Begin("Falcon 9 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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

    std::shared_ptr<ProcessingModule> FalconDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<FalconDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace falcon
