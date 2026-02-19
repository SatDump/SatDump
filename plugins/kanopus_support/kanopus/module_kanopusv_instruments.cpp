#include "module_kanopusv_instruments.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace kanopus
{
    KanopusVInstrumentsDecoderModule::KanopusVInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    void KanopusVInstrumentsDecoderModule::process()
    {
        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        uint8_t frm[1960];

        int img_n = 0;

        int counter_invalid = 0;

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)frm, 1960);

            uint16_t counter = ((frm[1946] & 0xF) << 8 | frm[1945]) >> 1;

            // Correct counter
            if (!counter_locked)
            {
                if (counter == ((global_counter + 1) % 495))
                {
                    logger->debug("Locked on counter : " + std::to_string(counter));
                    counter_locked = true;
                }
                else
                {
                    global_counter = counter;
                }
            }

            if (counter_locked)
            {
                int orig_counter = counter;
                counter = global_counter + 1;
                global_counter = counter;

                if (orig_counter != global_counter)
                    counter_invalid++;
                else
                    counter_invalid = 0;

                if (counter_invalid > 2)
                    counter_locked = false;
            }

            global_counter = counter % 495;

            // If we're locked, push into the next stuff
            if (counter_locked)
            {
                counter = global_counter;
                counter <<= 1;
                frm[1946] = (frm[1946] & 0xF0) | ((counter >> 8) & 0x0F);
                frm[1945] = (counter & 0b11111110) | (frm[1945] & 1);

                auto i = frame_proc.work(frm);

                if (i)
                {
                    image::save_img(i->img, directory + "/" + std::to_string(i->ccd_id) + "_" + std::to_string(++img_n) + ".png");

                    if (i->ccd_id == 1 || i->ccd_id == 2 || i->ccd_id == 3 || i->ccd_id == 4 || i->ccd_id == 5 || i->ccd_id == 6)
                    {
                        pmss_proc.proc_PSS(i->img.crop_to(4, 30, 1924, 495 - 2), i->ccd_id);
                    }

                    if (i->ccd_id == 7 || i->ccd_id == 8 || i->ccd_id == 9 || i->ccd_id == 10)
                    {
                        pmss_proc.proc_MSS(i->img.crop_to(4, 30, 1924, 495 - 2), i->ccd_id);
                    }
                }
            }
        }

        cleanup();

        {
            ////////////////////////////////////////// mtvza_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- MSS");
            //        logger->info("Lines : " + std::to_string(kmss_lines));

            satdump::products::ImageProduct kmss_product;
            kmss_product.instrument_name = "kanopus_mss";

            std::vector<satdump::ChannelTransform> transforms_def = {satdump::ChannelTransform().init_none(), satdump::ChannelTransform().init_none(), satdump::ChannelTransform().init_none(),
                                                                     satdump::ChannelTransform().init_none()};

            // if (sat_name == "METEOR-M2-4")
            //     transforms_def = {satdump::ChannelTransform().init_none(),                                     //
            //                       satdump::ChannelTransform().init_affine_slantx(1, 1, 4, -3.8, 4000, 0.0012), //
            //                       satdump::ChannelTransform().init_affine_slantx(1, 1, -2, -2, 4000, 0.0005)};

            kmss_product.images.push_back({0, "MSS-1", "1", pmss_proc.get_mss_full(0), 8, transforms_def[0]});
            kmss_product.images.push_back({0, "MSS-2", "2", pmss_proc.get_mss_full(1), 8, transforms_def[1]});
            kmss_product.images.push_back({0, "MSS-3", "3", pmss_proc.get_mss_full(2), 8, transforms_def[2]});
            kmss_product.images.push_back({0, "MSS-4", "4", pmss_proc.get_mss_full(3), 8, transforms_def[3]});

            kmss_product.save(directory);
            // dataset.products_list.push_back("KMSS_MSU100_1");

            ////////////////////////////////////////////////     mtvza_status = DONE;
        }

        std::string img_path = directory + "/pss.png";
        auto pss = pmss_proc.get_pss_full();
        image::save_img(pss, img_path);

#if 0
        logger->info("----------- OCM");
        logger->info("Lines : " + std::to_string(ocm_reader.lines));

        logger->info("Writing images.... (Can take a while)");

        if (!std::filesystem::exists(directory))
            std::filesystem::create_directory(directory);

        // Products dataset
        satdump::products::DataSet dataset;
        dataset.satellite_name = "OceanSat-2";
        dataset.timestamp = time(NULL); // avg_overflowless(avhrr_reader.timestamps);

        ocm_status = SAVING;

        satdump::products::ImageProduct ocm_products;
        ocm_products.instrument_name = "ocm_oc2";

        for (int i = 0; i < 8; i++)
            ocm_products.images.push_back({i, "OCM-" + std::to_string(i + 1), std::to_string(i + 1), ocm_reader.getChannel(i), 12});

        ocm_products.save(directory);
        dataset.products_list.push_back("OCM");

        ocm_status = DONE;

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
#endif
    }

    void KanopusVInstrumentsDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Kanopus-V Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTable("##kanopusvinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Instrument");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Lines / Frames");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Status");

            /* ImGui::TableNextRow();
             ImGui::TableSetColumnIndex(0);
             ImGui::Text("OCM");
             ImGui::TableSetColumnIndex(1);
             ImGui::TextColored(style::theme.green, "%d", ocm_reader.lines);
             ImGui::TableSetColumnIndex(2);
             drawStatus(ocm_status);*/
            ImGui::EndTable();
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string KanopusVInstrumentsDecoderModule::getID() { return "kanopusv_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> KanopusVInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<KanopusVInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace kanopus
