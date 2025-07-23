#include "module_oceansat_ocm.h"
#include "../../oceansat.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace oceansat
{
    namespace ocm
    {
        OceansatOCMDecoderModule::OceansatOCMDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void OceansatOCMDecoderModule::process()
        {

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OCM";

            uint8_t buffer[92220];

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)buffer, 92220);

                ocm_reader.work(buffer);
            }

            cleanup();

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

            /*
            logger->info("642 Composite...");
            {
                image::Image<uint16_t> image642(4072, ocm_reader.lines, 3);
                {
                    image642.draw_image(0, image6);
                    image642.draw_image(1, image4);
                    image642.draw_image(2, image2);
                }
                WRITE_IMAGE(image642, directory + "/OCM-RGB-642");
                image642.equalize();
                image642.normalize();
                WRITE_IMAGE(image642, directory + "/OCM-RGB-642-EQU");
                image::Image<uint16_t> corrected642 = image::earth_curvature::correct_earth_curvature(image642,
                                                                                                      OCEANSAT2_ORBIT_HEIGHT,
                                                                                                      OCEANSAT2_OCM2_SWATH,
                                                                                                      OCEANSAT2_OCM2_RES);
                WRITE_IMAGE(corrected642, directory + "/OCM-RGB-642-EQU-CORRECTED");
            }

            logger->info("654 Composite...");
            {
                image::Image<uint16_t> image654(4072, ocm_reader.lines, 3);
                {
                    image654.draw_image(0, image6);
                    image654.draw_image(1, image5);
                    image654.draw_image(2, image4);
                }
                WRITE_IMAGE(image654, directory + "/OCM-RGB-654");
                image654.equalize();
                image654.normalize();
                WRITE_IMAGE(image654, directory + "/OCM-RGB-654-EQU");
                image::Image<uint16_t> corrected654 = image::earth_curvature::correct_earth_curvature(image654,
                                                                                                      OCEANSAT2_ORBIT_HEIGHT,
                                                                                                      OCEANSAT2_OCM2_SWATH,
                                                                                                      OCEANSAT2_OCM2_RES);
                WRITE_IMAGE(corrected654, directory + "/OCM-RGB-654-EQU-CORRECTED");
            }

            logger->info("754 Composite...");
            {
                image::Image<uint16_t> image754(4072, ocm_reader.lines, 3);
                {
                    image754.draw_image(0, image7);
                    image754.draw_image(1, image5);
                    image754.draw_image(2, image4);
                }
                WRITE_IMAGE(image754, directory + "/OCM-RGB-754");
                image754.equalize();
                image754.normalize();
                WRITE_IMAGE(image754, directory + "/OCM-RGB-754-EQU");
                image::Image<uint16_t> corrected754 = image::earth_curvature::correct_earth_curvature(image754,
                                                                                                      OCEANSAT2_ORBIT_HEIGHT,
                                                                                                      OCEANSAT2_OCM2_SWATH,
                                                                                                      OCEANSAT2_OCM2_RES);
                WRITE_IMAGE(corrected754, directory + "/OCM-RGB-754-EQU-CORRECTED");
            }
            logger->info("882 Composite...");
            {
                image::Image<uint16_t> image882(4072, ocm_reader.lines, 3);
                {
                    image882.draw_image(0, image8);
                    image882.draw_image(1, image8);
                    image882.draw_image(2, image2);
                }
                WRITE_IMAGE(image882, directory + "/OCM-RGB-882");
                image882.equalize();
                image882.normalize();
                WRITE_IMAGE(image882, directory + "/OCM-RGB-882-EQU");
                image::Image<uint16_t> corrected882 = image::earth_curvature::correct_earth_curvature(image882,
                                                                                                      OCEANSAT2_ORBIT_HEIGHT,
                                                                                                      OCEANSAT2_OCM2_SWATH,
                                                                                                      OCEANSAT2_OCM2_RES);
                WRITE_IMAGE(corrected882, directory + "/OCM-RGB-882-EQU-CORRECTED");
            }

            logger->info("662 Composite...");
            {
                image::Image<uint16_t> image662(4072, ocm_reader.lines, 3);
                {
                    image662.draw_image(0, image6);
                    image662.draw_image(1, image6);
                    image662.draw_image(2, image2);
                }
                WRITE_IMAGE(image662, directory + "/OCM-RGB-662");
                image662.equalize();
                image662.normalize();
                WRITE_IMAGE(image662, directory + "/OCM-RGB-662-EQU");
                image::Image<uint16_t> corrected662 = image::earth_curvature::correct_earth_curvature(image662,
                                                                                                      OCEANSAT2_ORBIT_HEIGHT,
                                                                                                      OCEANSAT2_OCM2_SWATH,
                                                                                                      OCEANSAT2_OCM2_RES);
                WRITE_IMAGE(corrected662, directory + "/OCM-RGB-662-EQU-CORRECTED");
            }
            */
        }

        void OceansatOCMDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Oceansat OCM Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##oc2instrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OCM");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", ocm_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(ocm_status);
                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string OceansatOCMDecoderModule::getID() { return "oceansat_ocm"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> OceansatOCMDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<OceansatOCMDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ocm
} // namespace oceansat
