#include "module_stdc_parser.h"
#include "common/utils.h"
#include "egc_parser.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "msg_parser.h"
#include "pkt_parser.h"
#include "satdump_vars.h"
#include <filesystem>
#include <fstream>

namespace inmarsat
{
    namespace stdc
    {
        STDCParserModule::STDCParserModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            buffer = new uint8_t[FRAME_SIZE_BYTES * 4];
            memset(buffer, 0, FRAME_SIZE_BYTES * 4);

            if (parameters.contains("udp_sinks"))
            {
                for (auto &sinks : parameters["udp_sinks"].items())
                {
                    std::string address = sinks.value()["address"].get<std::string>();
                    int port = sinks.value()["port"].get<int>();
                    udp_clients.push_back(std::make_shared<net::UDPClient>((char *)address.c_str(), port));
                }
            }

            if (parameters.contains("save_files"))
                do_save_files = parameters["save_files"].get<bool>();
            else
                do_save_files = true;

            if (parameters.contains("station_id"))
                d_station_id = parameters["station_id"].get<std::string>();

            fsfsm_enable_output = false;
        }

        STDCParserModule::~STDCParserModule() { delete[] buffer; }

        std::string timestampToTod(time_t time_v)
        {
            std::tm *timeReadable = gmtime(&time_v);
            return (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" + // Hour HH
                   (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +    // Minutes mm
                   (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));           // Seconds ss
        }

        void STDCParserModule::process_final_pkt(nlohmann::json &msg)
        {
            // UDP
            {
                for (auto &c : udp_clients)
                {
                    try
                    {
                        nlohmann::json msg2 = msg;
                        if (d_station_id != "")
                        {
                            msg2["source"]["station_id"] = d_station_id;
                            msg2["source"]["app"]["name"] = "SatDump";
                            msg2["source"]["app"]["version"] = (std::string)satdump::SATDUMP_VERSION;
                        }

                        std::string m = msg2.dump();
                        c->send((uint8_t *)m.data(), m.size());
                    }
                    catch (std::exception &e)
                    {
                        logger->error("Error sending to UDP! %s", e.what());
                    }
                }
            }

            // File
            if (do_save_files)
            {
                std::string pkt_name = get_id_name(get_packet_frm_id(msg));
                if (msg.contains("pkt_name"))
                    pkt_name = msg["pkt_name"];

                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/" + pkt_name;

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                double time_d = msg["timestamp"].get<double>();
                time_t time_v = time_d;
                std::tm *timeReadable = gmtime(&time_v);

                std::string utc_filename = std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                           (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                           (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                           (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                           (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                           (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss

                std::ofstream outf(directory + "/" + utc_filename + ".json", std::ios::binary);
                outf << msg.dump(4);
                outf.close();
            }
        }

        void STDCParserModule::process()
        {
            STDPacketParser pkt_parser;
            MessageParser msg_parser;
            EGCMessageParser egc_parser;

            pkt_parser.on_packet = [&](nlohmann::json msg)
            {
                double time_d = msg["timestamp"].get<double>();
                int id = get_packet_frm_id(msg);

                if (id == pkts::PacketBulletinBoard::FRM_ID)
                {
                    msg_parser.push_current_time(time_d);
                    // egc_parser.push_current_time(time_d);
                }

                if (id != pkts::PacketMessageData::FRM_ID)
                    process_final_pkt(msg);

                if (id == pkts::PacketMessageData::FRM_ID)
                {
                    std::string m = msg["message"].get<std::string>();
                    logger->info("Message : \n%s", m.c_str());
                    if (m != "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ-!") // Remove those periodic ones
                        msg_parser.push_message(msg);
                }
                else if (id == pkts::PacketEGCDoubleHeader1::FRM_ID)
                {
                    logger->info("EGC Double Header 1 : \n%s", msg["message"].get<std::string>().c_str());
                    egc_parser.push_message(msg);
                }
                else if (id == pkts::PacketEGCDoubleHeader2::FRM_ID)
                {
                    logger->info("EGC Double Header 2 : \n%s", msg["message"].get<std::string>().c_str());
                    egc_parser.push_message(msg);
                }
                else
                    logger->info("Packet : " + get_id_name(id));

                if (is_gui)
                {
                    pkt_history_mtx.lock();
                    pkt_history.push_back(msg);
                    if (pkt_history.size() > 100)
                    {
                        pkt_history.erase(pkt_history.begin());
                        pkt_history.shrink_to_fit();
                    }
                    pkt_history_mtx.unlock();
                }

                // logger->info("Packet IDK : \n" + msg.dump(4));
            };

            msg_parser.on_message = [&](nlohmann::json msg)
            {
                msg["pkt_name"] = "Full Message";

                process_final_pkt(msg);

                logger->info("Full Message : \n%s", msg["message"].get<std::string>().c_str());

                if (is_gui)
                {
                    pkt_history_mtx.lock();
                    pkt_history_msg.push_back(msg);
                    if (pkt_history_msg.size() > 100)
                    {
                        pkt_history.erase(pkt_history.begin());
                        pkt_history.shrink_to_fit();
                    }
                    pkt_history_mtx.unlock();
                }
            };

            egc_parser.on_message = [&](nlohmann::json msg)
            {
                msg["pkt_name"] = "EGC Message";

                process_final_pkt(msg);

                logger->info("Full EGC Message : \n%s", msg["message"].get<std::string>().c_str());

                if (is_gui)
                {
                    pkt_history_mtx.lock();
                    pkt_history_egc.push_back(msg);
                    if (pkt_history_egc.size() > 100)
                    {
                        pkt_history_egc.erase(pkt_history_egc.begin());
                        pkt_history.shrink_to_fit();
                    }
                    pkt_history_mtx.unlock();
                }
            };

            while (should_run())
            {
                // Read a buffer
                read_data((uint8_t *)buffer, FRAME_SIZE_BYTES);

                last_pkt_count = buffer[2] << 8 | buffer[3];

                try
                {
                    // logger->critical("new pkt");
                    pkt_parser.parse_main_pkt(buffer);
                }
                catch (std::exception &e)
                {
                    logger->error("Error processing STD-C frame %s", e.what());
                }
            }

            egc_parser.force_finish();

            cleanup();
        }

        void STDCParserModule::drawUI(bool window)
        {
            is_gui = true;

            ImGui::Begin("Inmarsat STD-C Parser", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::Text("Decoded packets can be seen in a floating window.");
            ImGui::Text("Do remember you should not read nor keep messages that\nare not intended for you.");

            ImGui::Spacing();
            ImGui::Text("Last packet count : ");
            ImGui::SameLine();
            ImGui::TextColored(style::theme.green, "%d", last_pkt_count);

            drawProgressBar();

            ImGui::End();

            if (input_data_type != satdump::pipeline::DATA_FILE)
            {
                ImGui::Begin("STD-C Packets", NULL, ImGuiWindowFlags_HorizontalScrollbar);

                pkt_history_mtx.lock();
                ImGui::BeginTabBar("##sdtmessagestabbar");
                if (ImGui::BeginTabItem("Messages"))
                {
                    float wrap_pos = ImGui::GetContentRegionMax().x;
                    ImGui::BeginTable("##stdmessagetable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize, 150 * ui_scale);
                    ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_NoResize, 75 * ui_scale);
                    ImGui::TableSetupColumn("Contents", 0, -1);
                    ImGui::TableHeadersRow();

                    for (int i = pkt_history_msg.size() - 1; i >= 0; i--)
                    {
                        auto &msg = pkt_history_msg[i];
                        try
                        {
                            int id = get_packet_frm_id(msg);
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(style::theme.lavender, "%s", get_id_name(id).c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(style::theme.yellow, "%s", timestampToTod(msg["timestamp"].get<double>()).c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::PushTextWrapPos(wrap_pos);
                            ImGui::TextColored(style::theme.green, "%s", msg["message"].get<std::string>().c_str());
                            ImGui::PopTextWrapPos();
                        }
                        catch (std::exception &)
                        {
                        }
                    }
                    ImGui::EndTable();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("EGC Messages"))
                {
                    float wrap_pos = ImGui::GetContentRegionMax().x;
                    ImGui::BeginTable("##stdmessagetable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize, 150 * ui_scale);
                    ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_NoResize, 75 * ui_scale);
                    ImGui::TableSetupColumn("Contents", 0, -1);
                    ImGui::TableHeadersRow();

                    for (int i = pkt_history_egc.size() - 1; i >= 0; i--)
                    {
                        auto &msg = pkt_history_egc[i];
                        try
                        {
                            int id = get_packet_frm_id(msg);
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(style::theme.lavender, "%s", get_id_name(id).c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(style::theme.yellow, "%s", timestampToTod(msg["timestamp"].get<double>()).c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::PushTextWrapPos(wrap_pos);
                            ImGui::TextColored(style::theme.green, "%s", msg["message"].get<std::string>().c_str());
                            ImGui::PopTextWrapPos();
                        }
                        catch (std::exception &)
                        {
                        }
                    }
                    ImGui::EndTable();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Packets"))
                {
                    float wrap_pos = ImGui::GetContentRegionMax().x;
                    ImGui::BeginTable("##stdmessagetable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize, 150 * ui_scale);
                    ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_NoResize, 75 * ui_scale);
                    ImGui::TableSetupColumn("Contents", 0, -1);
                    ImGui::TableHeadersRow();

                    for (int i = pkt_history.size() - 1; i >= 0; i--)
                    {
                        auto &msg = pkt_history[i];
                        try
                        {
                            int id = get_packet_frm_id(msg);
                            if (id == pkts::PacketMessageData::FRM_ID)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextColored(style::theme.lavender, "%s", get_id_name(id).c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(style::theme.yellow, "%s", timestampToTod(msg["timestamp"].get<double>()).c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::PushTextWrapPos(wrap_pos);
                                ImGui::TextColored(style::theme.green, "%s", msg["message"].get<std::string>().c_str());
                                ImGui::PopTextWrapPos();
                            }
                            else if (id == pkts::PacketEGCDoubleHeader1::FRM_ID || id == pkts::PacketEGCDoubleHeader2::FRM_ID)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextColored(style::theme.lavender, "%s", get_id_name(id).c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(style::theme.yellow, "%s", timestampToTod(msg["timestamp"].get<double>()).c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::PushTextWrapPos(wrap_pos);
                                ImGui::TextColored(style::theme.red, "%s", msg["message"].get<std::string>().c_str());
                                ImGui::PopTextWrapPos();
                            }
                            else
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextColored(style::theme.lavender, "%s", get_id_name(id).c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextColored(style::theme.yellow, "%s", timestampToTod(msg["timestamp"].get<double>()).c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::PushTextWrapPos(wrap_pos);
                                ImGui::TextUnformatted(msg.dump().c_str());
                                ImGui::PopTextWrapPos();
                            }
                        }
                        catch (std::exception &)
                        {
                        }
                    }
                    ImGui::EndTable();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
                pkt_history_mtx.unlock();

                ImGui::End();
            }
        }

        std::string STDCParserModule::getID() { return "inmarsat_stdc_parser"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> STDCParserModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<STDCParserModule>(input_file, output_file_hint, parameters);
        }
    } // namespace stdc
} // namespace inmarsat