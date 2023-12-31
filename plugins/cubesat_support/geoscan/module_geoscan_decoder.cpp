#include "module_geoscan_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/image.h"

#include "../common/scrambling.h"

namespace geoscan
{
    GEOSCANDecoderModule::GEOSCANDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        input_buffer = new int8_t[256];
    }

    GEOSCANDecoderModule::~GEOSCANDecoderModule()
    {
        delete[] input_buffer;
    }

    std::vector<ModuleDataType> GEOSCANDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> GEOSCANDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    // Compare 2 32-bits values and return the difference count
    inline int compare_32(uint32_t v1, uint32_t v2)
    {
        int cor = 0;
        uint32_t diff = v1 ^ v2;
        for (; diff; cor++)
            diff &= diff - 1;
        return cor;
    }

    void GEOSCANDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        uint32_t syncword_shift;
        uint8_t byte_shifter;
        int bitshift_in_byte = 0;
        bool in_frame = false;
        std::vector<uint8_t> framebuf;

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)input_buffer, 256);
            else
                input_fifo->read((uint8_t *)input_buffer, 256);

            for (int i = 0; i < 256; i++)
            {
                // Slice
                uint8_t bit = input_buffer[i] > 0;

                syncword_shift = syncword_shift << 1 | bit;

                if (in_frame)
                {
                    byte_shifter = byte_shifter << 1 | bit;
                    bitshift_in_byte++;

                    if (bitshift_in_byte == 8)
                    {
                        bitshift_in_byte = 0;
                        framebuf.push_back(byte_shifter);
                    }

                    if (framebuf.size() == 70)
                    {
                        in_frame = false;

                        const uint8_t pn9_scrambling[] = {0xFF, 0xE1, 0x1D, 0x9A, 0xED, 0x85, 0x33, 0x24, 0xEA, 0x7A, 0xD2, 0x39, 0x70, 0x97, 0x57, 0x0A, 0x54, 0x7D, 0x2D, 0xD8, 0x6D, 0x0D, 0xBA, 0x8F, 0x67, 0x59, 0xC7, 0xA2, 0xBF, 0x34, 0xCA, 0x18, 0x30, 0x53, 0x93, 0xDF, 0x92, 0xEC, 0xA7, 0x15, 0x8A, 0xDC, 0xF4, 0x86, 0x55, 0x4E, 0x18, 0x21, 0x40, 0xC4, 0xC4, 0xD5, 0xC6, 0x91, 0x8A, 0xCD, 0xE7, 0xD1, 0x4E, 0x09, 0x32, 0x17, 0xDF, 0x83, 0xFF, 0xF0};

                        for (int i = 0; i < 66; i++)
                            framebuf[4 + i] ^= pn9_scrambling[i];

                        uint16_t crc_frm1 = crc_check.compute(&framebuf[4], 64);
                        uint16_t crc_frm2 = framebuf[4 + 64 + 0] << 8 | framebuf[4 + 64 + 1];

                        if (crc_frm1 == crc_frm2)
                        {
                            data_out.write((char *)framebuf.data(), framebuf.size());
                            frm_cnt++;
                        }
                    }

                    continue;
                }

                if (compare_32(syncword_shift, 0x930B51DE) < 3)
                {
                    framebuf.clear();
                    in_frame = true;
                    framebuf.push_back(0x93);
                    framebuf.push_back(0x0B);
                    framebuf.push_back(0x51);
                    framebuf.push_back(0xDE);
                }

#if 0
                byte_shifter = byte_shifter << 1 | bit;
                bitshift_in_byte++;

                if (bitshift_in_byte)
#endif
            }

#if 0
            for (auto frm : frames)
            {
                std::string source_callsign;
                for (int i = 0; i < 6; i++)
                    source_callsign += frm[i] >> 1;

                std::string destination_callsign;
                for (int i = 7; i < 7 + 6; i++)
                    destination_callsign += frm[i] >> 1;

                int source_ssid = frm[6] >> 1;
                int target_ssid = frm[7 + 6] >> 1;

                int ctrl = frm[14];
                int pid = frm[15];

                logger->trace("AX.25 Frame - SRC : " + source_callsign + ",%d DST : " + destination_callsign + ",%d CTRL : %d PID : %d",
                              source_ssid, target_ssid, ctrl, pid);

                // Add a custom header to ease up forwarding to the next stuff
                uint16_t pkt_sz = frm.size();

                uint8_t buffer_sz[6];
                buffer_sz[0] = 0x1a;
                buffer_sz[1] = 0xcf;
                buffer_sz[2] = 0xfc;
                buffer_sz[3] = 0x1d;
                buffer_sz[4] = pkt_sz >> 8;
                buffer_sz[5] = pkt_sz & 0xFF;

                frm.insert(frm.begin(), buffer_sz, buffer_sz + 6);
                data_out.write((char *)frm.data(), frm.size());

                frm_cnt++;
            }
#endif

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Frames " + std::to_string(frm_cnt));
            }
        }

        logger->info("Decoding finished");

        data_in.close();
    }

    void GEOSCANDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("GeoScan Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frm_cnt));
        }

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string GEOSCANDecoderModule::getID()
    {
        return "geoscan_decoder";
    }

    std::vector<std::string> GEOSCANDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> GEOSCANDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GEOSCANDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
