#include "apid_sender.h"
#include <map>

namespace apid
{
    APIDSenderModule::APIDSenderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        save_to_cadu = false;
        if (parameters.count("server_address") > 0)
            addr = parameters["server_address"].get<std::string>();
        else
            throw satdump_exception("server_address parameter must be present!");

        if (!parameters.contains("apids") || !parameters["apids"].is_array() || parameters["apids"].empty())
            throw satdump_exception("apids array must be present and non-empty!");

        for (auto &entry : parameters["apids"])
        {
            if (!entry.contains("apid") || !entry.contains("port"))
                throw satdump_exception("Each apid entry must have 'apid' and 'port' fields!");

            ApidConfig cfg;
            cfg.apid = entry["apid"].get<int>();
            cfg.port = entry["port"].get<int>();
            cfg.packet_size = entry.value("packet_size", -1); // -1 = use pkt.payload.size()
            apid_configs.push_back(cfg);
        }

        if (parameters.count("fecf") > 0)
            use_fecf = parameters["fecf"].get<bool>();
        else
            use_fecf = false;

        if (parameters.count("save_cadu") > 0)
            save_to_cadu = parameters["save_cadu"].get<bool>();

        fsfsm_enable_output = save_to_cadu;
        fsfsm_file_ext = ".cadu";

        int interleaving = 4;
        if (parameters.count("rs_i") > 0)
            interleaving = parameters["rs_i"].get<int>();

        if (interleaving > 0)
        {
            cadu_size = (4 + interleaving * 255) * 8;
            mpdu_data_size = (interleaving * 223) - 8 - (use_fecf ? 2 : 0);
        }
        else
        {
            if (parameters.count("cadu_size") > 0)
                cadu_size = parameters["cadu_size"].get<int>();
            else
                cadu_size = 8192;

            if (parameters.count("mpdu_size") > 0)
                mpdu_data_size = parameters["mpdu_size"].get<int>();
            else
                mpdu_data_size = (use_fecf) ? 882 : 884;
        }
    }

    void APIDSenderModule::process()
    {
        uint8_t *cadu;
        cadu = (uint8_t *)malloc(cadu_size / 8);

        // Build one UDP client per configured APID
        // UDPClient is non-copyable and non-movable, so store behind unique_ptr
        std::vector<std::unique_ptr<net::UDPClient>> udp_senders;
        udp_senders.reserve(apid_configs.size());
        for (auto &cfg : apid_configs)
            udp_senders.push_back(std::make_unique<net::UDPClient>((char *)addr.c_str(), cfg.port));

        logger->info("Meow :3");
        logger->info("Demultiplexing and deframing...");

        ccsds::ccsds_aos::Demuxer demuxer_vcid0(mpdu_data_size, false);

        while (should_run())
        {
            read_data((uint8_t *)cadu, cadu_size / 8);

            if (save_to_cadu)
            {
                write_data(cadu, cadu_size / 8);
            }

            ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

            if (vcdu.vcid == 0)
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                {
                    for (size_t i = 0; i < apid_configs.size(); ++i)
                    {
                        ApidConfig &cfg = apid_configs[i];
                        if (pkt.header.apid != cfg.apid)
                            continue;

                        // If packet_size is set, only send if payload matches
                        if (cfg.packet_size >= 0 && static_cast<int>(pkt.payload.size()) != cfg.packet_size)
                            continue;

                        int send_size = (cfg.packet_size >= 0) ? cfg.packet_size : static_cast<int>(pkt.payload.size());

                        static std::map<int, int> err_cnts;
                        try
                        {
                            udp_senders[i]->send(pkt.payload.data(), send_size);
                            err_cnts.clear(); // Reset all counters if any packet sends successfully
                            cfg.total_pkts++; // Increment counter
                        }
                        catch (std::exception &e)
                        {
                            int &err_cnt = err_cnts[cfg.apid];
                            if (err_cnt < 5)
                            {
                                logger->error("UDP Send Error (APID %d): %s", cfg.apid, e.what());
                                err_cnt++;
                                if (err_cnt == 5)
                                    logger->error("Suppressing further UDP Send Errors for APID %d to avoid log spam.", cfg.apid);
                            }
                        }

                        break; // Matched — no need to check further configs
                    }
                }
            }
        }

        free(cadu);
        cleanup();
    }

    void APIDSenderModule::drawUI(bool window)
    {
        ImGui::Begin("Network Server", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Text("Address  : ");
        ImGui::SameLine();
        ImGui::TextColored(style::theme.green, "%s", addr.c_str());

        ImGui::Separator();

        if (ImGui::BeginTable("##apid_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("APID");
            ImGui::TableSetupColumn("Port");
            ImGui::TableSetupColumn("Pkts / 10s");
            ImGui::TableHeadersRow();

            for (auto &cfg : apid_configs)
            {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - cfg.last_history_update).count() >= 1) {
                    cfg.history_pkts[cfg.history_idx] = cfg.total_pkts.load();
                    cfg.history_idx = (cfg.history_idx + 1) % 10;
                    cfg.last_history_update = now;
                }

                uint64_t current_pkts = cfg.total_pkts.load();
                uint64_t oldest_pkts = cfg.history_pkts[cfg.history_idx]; // The oldest record is the next one to be overwritten
                uint64_t pkts_last_10s = current_pkts - oldest_pkts;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", cfg.apid);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", cfg.port);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(style::theme.green, "%llu", (unsigned long long)pkts_last_10s);
            }

            ImGui::EndTable();
        }

        if (!d_is_streaming_input)
            drawProgressBar();

        ImGui::End();
    }

    std::string APIDSenderModule::getID() { return "apid_sender"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> APIDSenderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    { return std::make_shared<APIDSenderModule>(input_file, output_file_hint, parameters); }

} // namespace apid
