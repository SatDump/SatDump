#include "module_cluster_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_standard/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_standard/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"
#include "common/simple_deframer.h"
#include "instruments/wbd_decoder.h"
#include "common/audio/audio_sink.h"

namespace cluster
{
    namespace instruments
    {
        CLUSTERInstrumentsDecoderModule::CLUSTERInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void CLUSTERInstrumentsDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
            std::ifstream data_in;
            if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1279];

            // Demuxers
            ccsds::ccsds_standard::Demuxer demuxer_vcid1(1101, false);

            def::SimpleDeframer wbddeframer(0xFAF334, 24, 8768, 0);


            std::ofstream output("file.ccsds");
            WBDdecoder wbddecode;


            bool enable_audio = false;
            std::shared_ptr<audio::AudioSink> audio_sink;
                if (input_data_type != DATA_FILE && audio::has_sink())
            {
                enable_audio = true;
                audio_sink = audio::get_default_sink();
                audio_sink->set_samplerate(27443);
                audio_sink->start();
            }            

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&cadu, 1279);
                else
                    input_fifo->read((uint8_t *)&cadu, 1279);

                // Read buffer

                // Parse this transport frame
                ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 5) // Parse VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        std::vector<std::vector<uint8_t>> minorframes = wbddeframer.work(pkt.payload.data(), pkt.payload.size());
                        for(auto &frm : minorframes){
                            std::vector<MajorFrame> majorframes = wbddecode.work(frm.data());
                            for(auto &majorframe : majorframes){
                                int16_t audiobuf[4352];
                                for(int i = 0; i < 4352; i++){
                                    audiobuf[i] = (int(majorframe.payload[i]) - 127) * 255;
                                }

                                if(enable_audio){
                                    audio_sink->push_samples(audiobuf,4352);
                                }
                                if(majorframe.VCXO == 0 && majorframe.antennaselect == 0){
                                     output.write((char *)majorframe.payload.data(),majorframe.payload.size());
                                     

                                }
                            }
                        }
                    }
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }
            if (input_data_type == DATA_FILE)

            data_in.close();

            // dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void CLUSTERInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Cluster Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);
// /
            // /if (ImGui::BeginTable("##clusterinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            // /{
            // /    ImGui::TableNextRow();
            // /    ImGui::TableSetColumnIndex(0);
            // /    ImGui::Text("Instrument");
            // /    ImGui::TableSetColumnIndex(1);
            // /    ImGui::Text("Images / Frames");
            // /    ImGui::TableSetColumnIndex(2);
            // /    ImGui::Text("Status");
// /
            // /    for (int i = 0; i < 4; i++)
            // /    {
            // /        ImGui::TableNextRow();
            // /        ImGui::TableSetColumnIndex(0);
            // /        ImGui::Text("CIPS %d", i + 1);
            // /        ImGui::TableSetColumnIndex(1);
            // /        ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)cips_readers[i].images.size());
            // /        ImGui::TableSetColumnIndex(2);
            // /        drawStatus(cips_status[i]);
            // /    }
// /
            // /    ImGui::EndTable();
            // /}
// /
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string CLUSTERInstrumentsDecoderModule::getID()
        {
            return "cluster_instruments";
        }

        std::vector<std::string> CLUSTERInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> CLUSTERInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CLUSTERInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop