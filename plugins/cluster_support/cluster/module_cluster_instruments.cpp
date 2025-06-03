#include "module_cluster_instruments.h"
#include "common/audio/audio_sink.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/dsp/io/wav_writer.h"
#include "common/simple_deframer.h"
#include "common/utils.h"
#include "core/config.h"
#include "imgui/imgui.h"
#include "instruments/wbd_decoder.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace cluster
{
    namespace instruments
    {
        CLUSTERInstrumentsDecoderModule::CLUSTERInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            play_audio = satdump::config::main_cfg["user_interface"]["play_audio"]["value"].get<bool>();
            fsfsm_enable_output = false;
        }

        void CLUSTERInstrumentsDecoderModule::process()
        {
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_tm::Demuxer demuxer_vcid1(1101, false);

            std::ofstream output(d_output_file_hint + "_wbd.frm", std::ios::binary);
            std::ofstream output_cadu2(d_output_file_hint + "_cadu_bkp.cadu", std::ios::binary);

            def::SimpleDeframer wbddeframer(0xFAF334, 24, 8768, 0);
            WBDdecoder wbddecode;

            // Audio stuff
            int16_t audio_buffer[4352];
            std::shared_ptr<audio::AudioSink> audio_sink;
            if (input_data_type != satdump::pipeline::DATA_FILE && audio::has_sink())
            {
                enable_audio = true;
                audio_sink = audio::get_default_sink();
                audio_sink->set_samplerate(27443);
                audio_sink->start();
            }

            // Buffers to wav...for all the antennas
            std::ofstream out_antenna_Ez(d_output_file_hint + "_Ez.wav", std::ios::binary);
            uint64_t final_data_size_ant_Ez = 0;
            dsp::WavWriter wave_writer_Ez(out_antenna_Ez);
            wave_writer_Ez.write_header(27443, 1);

            std::ofstream out_antenna_Bx(d_output_file_hint + "_Bx.wav", std::ios::binary);
            uint64_t final_data_size_ant_Bx = 0;
            dsp::WavWriter wave_writer_Bx(out_antenna_Bx);
            wave_writer_Bx.write_header(27443, 1);

            std::ofstream out_antenna_By(d_output_file_hint + "_By.wav", std::ios::binary);
            uint64_t final_data_size_ant_By = 0;
            dsp::WavWriter wave_writer_By(out_antenna_By);
            wave_writer_By.write_header(27443, 1);

            std::ofstream out_antenna_Ey(d_output_file_hint + "_Ey.wav", std::ios::binary);
            uint64_t final_data_size_ant_Ey = 0;
            dsp::WavWriter wave_writer_Ey(out_antenna_Ey);
            wave_writer_Ey.write_header(27443, 1);

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1279);

                // Save (for now)
                output_cadu2.write((char *)cadu, 1279);

                // Parse this transport frame
                ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

                if (vcdu.vcid == 5) // Parse WBD VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        std::vector<std::vector<uint8_t>> minorframes = wbddeframer.work(pkt.payload.data(), pkt.payload.size());
                        for (auto &frm : minorframes)
                        {
                            std::vector<MajorFrame> majorframes = wbddecode.work(frm.data());
                            for (auto &majorframe : majorframes)
                            {
                                for (int i = 0; i < 4352; i++)
                                    audio_buffer[i] = (int(majorframe.payload[i]) - 127) * 255;

                                if (enable_audio && play_audio)
                                    audio_sink->push_samples(audio_buffer, 4352);

                                if (majorframe.VCXO == 0)
                                {
                                    output.write((char *)majorframe.payload.data(), majorframe.payload.size());
                                    if (majorframe.antennaselect == 0)
                                    {
                                        out_antenna_Ez.write((char *)audio_buffer, 4352 * sizeof(int16_t));
                                        final_data_size_ant_Ez += 4352 * sizeof(int16_t);
                                    }
                                    else if (majorframe.antennaselect == 1)
                                    {
                                        out_antenna_Bx.write((char *)audio_buffer, 4352 * sizeof(int16_t));
                                        final_data_size_ant_Bx += 4352 * sizeof(int16_t);
                                    }
                                    else if (majorframe.antennaselect == 2)
                                    {
                                        out_antenna_By.write((char *)audio_buffer, 4352 * sizeof(int16_t));
                                        final_data_size_ant_By += 4352 * sizeof(int16_t);
                                    }
                                    else if (majorframe.antennaselect == 3)
                                    {
                                        out_antenna_Ey.write((char *)audio_buffer, 4352 * sizeof(int16_t));
                                        final_data_size_ant_Ey += 4352 * sizeof(int16_t);
                                    }
                                }

                                param_antenna = majorframe.antennaselect;
                                param_convfrq = majorframe.convfreq;
                                param_band = majorframe.outputmode;
                                param_VCXO = majorframe.VCXO;
                                param_OBDH = majorframe.OBDH;
                                param_commands = majorframe.commands;
                            }
                        }
                    }
                }
            }

            if (enable_audio)
                audio_sink->stop();

            wave_writer_Ez.finish_header(final_data_size_ant_Ez);
            wave_writer_Bx.finish_header(final_data_size_ant_Bx);
            wave_writer_By.finish_header(final_data_size_ant_By);
            wave_writer_Ey.finish_header(final_data_size_ant_Ey);
            out_antenna_Ez.close();
            out_antenna_Bx.close();
            out_antenna_By.close();
            out_antenna_Ey.close();

            cleanup();
            output_cadu2.close();
        }

        void CLUSTERInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Cluster Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::Text("Antenna :");
            ImGui::SameLine();
            if (param_antenna == 0)
                ImGui::Text("Ez");
            else if (param_antenna == 1)
                ImGui::Text("Bx");
            else if (param_antenna == 2)
                ImGui::Text("By");
            else if (param_antenna == 3)
                ImGui::Text("Ey");

            ImGui::Text("Conversion Freq : ");
            ImGui::SameLine();
            if (param_convfrq == 0)
                ImGui::Text("Baseband");
            else if (param_convfrq == 1)
                ImGui::Text("125.454 kHz");
            else if (param_convfrq == 2)
                ImGui::Text("250.908 kHz");
            else if (param_convfrq == 3)
                ImGui::Text("501.816 kHz");

            // ImGui::Text("Freq Band:");
            // ImGui::SameLine();
            // ImGui::Text("%d", param_band);

            ImGui::Text("VCXO : ");
            ImGui::SameLine();
            if (param_VCXO == 0)
                ImGui::Text("Locked");
            else
                ImGui::Text("Unlocked");

            ImGui::Text("OBDH : ");
            ImGui::SameLine();
            ImGui::Text("%d", param_OBDH);

            ImGui::Text("CMD : ");
            ImGui::SameLine();
            ImGui::Text("%d", param_commands);

            if (enable_audio)
            {
                ImGui::Spacing();
                const char *btn_icon, *label;
                ImU32 color;
                if (play_audio)
                {
                    color = style::theme.green;
                    btn_icon = u8"\uF028##wbdaudio";
                    label = "Audio Playing";
                }
                else
                {
                    color = style::theme.red;
                    btn_icon = u8"\uF026##wbdaudio";
                    label = "Audio Muted";
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                if (ImGui::Button(btn_icon))
                    play_audio = !play_audio;
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextUnformatted(label);
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string CLUSTERInstrumentsDecoderModule::getID() { return "cluster_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> CLUSTERInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CLUSTERInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace cluster
// For Michal B. : This is what i am doing in your classes instead of boring python tasks :) -RS