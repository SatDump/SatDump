#include "ssdv_decode.h"

namespace ssdv
{
    SSDVInstrumentsDecoderModule::SSDVInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        // if (parameters.contains("pkt_size") > 0)
        //     pkt_size = parameters["pkt_size"].get<int>();
        // else
        //     throw satdump_exception("pkt_size parameter must be present!");

        if (parameters.count("server_address") > 0)
            addr = parameters["server_address"].get<std::string>();
        else
            throw satdump_exception("server_address parameter must be present!");

        if (parameters.count("server_port_apid_10") > 0)
            port_apid_10 = parameters["server_port_apid_10"].get<int>();
        else
            throw satdump_exception("server_port_apid_10 parameter must be present!");

        if (parameters.count("server_port_apid_20") > 0)
            port_apid_20 = parameters["server_port_apid_20"].get<int>();
        else
            throw satdump_exception("server_port_apid_20 parameter must be present!");

        if (parameters.count("server_port_apid_100") > 0)
            port_apid_100 = parameters["server_port_apid_100"].get<int>();
        else
            throw satdump_exception("server_port_apid_100 parameter must be present!");

        if (parameters.count("fecf") > 0)
            use_fecf = parameters["fecf"].get<bool>();
        else
            use_fecf = false;
    }

    void SSDVInstrumentsDecoderModule::process()
    {
        uint8_t cadu[1024];

        net::UDPClient udp_apid_10_send((char *)addr.c_str(), port_apid_10);
        net::UDPClient udp_apid_20_send((char *)addr.c_str(), port_apid_20);
        net::UDPClient udp_apid_100_send((char *)addr.c_str(), port_apid_100);

        logger->info("Meow :3");
        logger->info("Demultiplexing and deframing...");

        ccsds::ccsds_aos::Demuxer demuxer_vcid0((use_fecf) ? 882 : 884, false);

        while (should_run())
        {
            read_data((uint8_t *)cadu, 1024);

            ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

            if (vcdu.vcid == 0)
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 10 && pkt.payload.size() == 246)
                    {
                        udp_apid_10_send.send(pkt.payload.data(), 246);
                        ssdv_ng_reader.work(pkt);
                        // pkt.payload.resize(65536);
                        // output.write((char *)pkt.header.raw, 6);
                        // output.write((char *)pkt.payload.data(), 1024);
                    }
                    else if (pkt.header.apid == 20 && pkt.payload.size() == 752) // We fit 4 MPEG TS packets in there, so 752 bytes of payload
                    {
                        // output.write((char *)pkt.payload.data(), pkt.payload.size());
                        udp_apid_20_send.send(pkt.payload.data(), 752);
                    }
                    else if (pkt.header.apid == 100)
                    {
                        udp_apid_100_send.send(pkt.payload.data(), 10);
                    }
            }
        }

        // delete[] net_buf;

        cleanup();

        // int scid = satdump::most_common(ssdv_scids.begin(), ssdv_scids.end(), 0);
        // ssdv_scids.clear();
        //
        // std::string sat_name = "Unkown SSDV";
        // if (scid == SSDV_SCID)
        //     norad = SSDV_NORAD;

        // satdump::products::DataSet dataset;
        // dataset.satellite_name = "SSDV";

        // dataset.timestamp = satdump::get_median(nn_reader.timestamps);
    }

    void SSDVInstrumentsDecoderModule::drawUI(bool window)
    {
        /*
        ImGui::Begin("SSDV Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTable("##ssdvinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Instrument");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Images / Frames");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Status");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("SSDV Images");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", ssdv_ng_reader.img_cnt);
            ImGui::TableSetColumnIndex(2);
            drawStatus(ssdv_ng_status);

            // TODOREWORK: table with packets sent?

            ImGui::EndTable();
        }

        if (!d_is_streaming_input)
            drawProgressBar();

        ImGui::End();
        */

        ImGui::Begin("Network Server", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Text("Address  : ");
        ImGui::SameLine();
        ImGui::TextColored(style::theme.green, "%s", addr.c_str());

        ImGui::Text("Port For APID 10    : ");
        ImGui::SameLine();
        ImGui::TextColored(style::theme.green, UITO_C_STR(port_apid_10));

        ImGui::Text("Port For APID 20    : ");
        ImGui::SameLine();
        ImGui::TextColored(style::theme.green, UITO_C_STR(port_apid_20));

        ImGui::Text("Port For APID 100    : ");
        ImGui::SameLine();
        ImGui::TextColored(style::theme.green, UITO_C_STR(port_apid_100));

        ImGui::End();
    }

    std::string SSDVInstrumentsDecoderModule::getID() { return "ssdv_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> SSDVInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    { return std::make_shared<SSDVInstrumentsDecoderModule>(input_file, output_file_hint, parameters); }

} // namespace ssdv
