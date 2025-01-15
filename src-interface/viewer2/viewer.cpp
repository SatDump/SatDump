#include "viewer.h"
// #include "image_handler.h"
// #include "radiation_handler.h"
// #include "scatterometer_handler.h"
// #include "core/config.h"
// #include "products/dataset.h"
#include "common/utils.h"
// #include "resources.h"
#include "main_ui.h"

#include "handler/dummy_handler.h"
#include "handler/product/image_product_handler.h" // TODOREWORK CLEAN
#include "handler/dataset/dataset_handler.h"

namespace satdump
{
    namespace viewer
    {
        ViewerApplication::ViewerApplication()
            : Application("viewer")
        {
            master_handler = std::make_shared<DummyHandler>("MasterHandlerViewer");
        }

        ViewerApplication::~ViewerApplication()
        {
            if (file_open_thread.joinable())
                file_open_thread.join();
        }

        void ViewerApplication::drawPanel()
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginTable("##masterhandlertable", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("##masterhandlertable_col", ImGuiTableColumnFlags_None);
                    ImGui::TableNextColumn();

                    master_handler->drawTreeMenu(curr_handler);

                    ImGui::EndTable();
                }

                if (curr_handler)
                    curr_handler->drawMenu();
            }
        }

        void ViewerApplication::drawMenuBar()
        {
            // Handle File Stuff TODOREWORK?
            {
                if (file_open_dialog && file_open_dialog->ready(0))
                {
                    std::string prod_path = (file_open_dialog->result().size() == 0 ? "" : file_open_dialog->result()[0]);
                    delete file_open_dialog;
                    file_open_dialog = nullptr;
                    openProductOrDataset(prod_path);
                }
            }

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open Product/Dataset (.cbor/.json)") && !file_open_dialog)
                        file_open_dialog = new pfd::open_file("Dataset/Product (.cbor/.json)", ".", {"CBOR Files", "*.cbor", "JSON Files", "*.json"}, pfd::opt::force_path); // TODOREWORK remember path?

                    ImGui::MenuItem("Open Image");

                    if (ImGui::BeginMenu("Hardcoded"))
                    {
                        if (ImGui::MenuItem("Load KMSS"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/dataset.json");
                        if (ImGui::MenuItem("Load Sterna"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/aws_pfm_cadu/dataset.json");
                        if (ImGui::MenuItem("Load MSUGS"))
                            openProductOrDataset("/home/alan/Downloads/20241231_132953_ARKTIKA-M 2_dat/MSUGS_VIS1/product.cbor");
                        if (ImGui::MenuItem("Load MSUGS 2"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/20250104_071415_ELEKTRO-L_3_dat/dataset.json");
                        if (ImGui::MenuItem("Load MetOp"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/metop_test/dataset.json");
                        if (ImGui::MenuItem("Load L3"))
                            openProductOrDataset("/data_ssd/ELEKTRO-L3/20250104_071415_ELEKTRO-L 3.dat_OUT/dataset.json");
                        if (ImGui::MenuItem("Load JPSS-1"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/202411271228_NOAA_20/dataset.json");
                        if (ImGui::MenuItem("Load JPSS-2"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/n21_day/dataset.json");
                        if (ImGui::MenuItem("Load APT"))
                            openProductOrDataset("/home/alan/Downloads/audio_137912500Hz_11-39-56_04-03-2024_wav/dataset.json");
                        if (ImGui::MenuItem("Load GOES"))
                            openProductOrDataset("/home/alan/Downloads/SatDump_NEWPRODS/goes_hrit_jvital2013_cadu/IMAGES/GOES-16/Full Disk/2024-04-17_18-00-20/product.cbor");

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Handler"))
                {
                    if (ImGui::BeginMenu("Add"))
                    {
                        ImGui::MenuItem("Dataset");
                        ImGui::MenuItem("Projection");
                        ImGui::EndMenu();
                    }
                    if (curr_handler && ImGui::BeginMenu("Config"))
                    {
                        if (ImGui::MenuItem("JSON To Clipboard"))
                            ImGui::SetClipboardText(curr_handler->getConfig().dump(4).c_str());
                        if (ImGui::MenuItem("JSON From Clipboard"))
                            curr_handler->setConfig(nlohmann::json::parse(ImGui::GetClipboardText()));
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                if (curr_handler)
                {
                    if (ImGui::BeginMenu("Current"))
                    {
                        curr_handler->drawMenuBar();
                        ImGui::EndMenu();
                    }
                }

                ImGui::EndMenuBar();
            }
        }

        void ViewerApplication::drawContents()
        {
            ImGui::Text("No handler selected!");
        }

        void ViewerApplication::drawUI()
        {
            ImVec2 viewer_size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginTable("##wiever_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, viewer_size.x * panel_ratio);
                ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, viewer_size.x * (1.0f - panel_ratio));
                ImGui::TableNextColumn();

                float left_width = ImGui::GetColumnWidth(0);
                float right_width = viewer_size.x - left_width;
                if (left_width != last_width && last_width != -1)
                    panel_ratio = left_width / viewer_size.x;
                last_width = left_width;

                ImGui::BeginChild("ViewerChildPanel", {left_width, float(viewer_size.y - 10)}, false, ImGuiWindowFlags_MenuBar);
                drawMenuBar();
                drawPanel();
                ImGui::EndChild();

                ImGui::TableNextColumn();
                ImGui::BeginGroup();

                if (curr_handler)
                    curr_handler->drawContents({float(right_width - 4), float(viewer_size.y)});
                else
                    drawContents();

                ImGui::EndGroup();
                ImGui::EndTable();
            }
        }

        void ViewerApplication::openProductOrDataset(std::string path) // TODOREWORK
        {
            if (file_open_thread.joinable())
                file_open_thread.join();
            auto fun = [this, path]()
            {
                if (!std::filesystem::path(path).has_extension())
                {
                    logger->error("Invalid file, no extension!");
                    return;
                }

                // TODOREWORK Load more than just image products
                if (std::filesystem::path(path).extension().string() == ".cbor")
                {
                    try
                    {
                        std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct(path));
                        master_handler->addSubHandler(prod_h);
                    }
                    catch (std::exception &e)
                    {
                        logger->error("Error loading product! Maybe not a valid product? Details : %s", e.what());
                    }
                }
                else if (std::filesystem::path(path).extension().string() == ".json")
                {
                    try
                    {
                        products::DataSet dataset;
                        dataset.load(path);
                        std::shared_ptr<DatasetHandler> dat_h = std::make_shared<DatasetHandler>(std::filesystem::path(path).parent_path().string(), dataset);
                        master_handler->addSubHandler(dat_h);
                    }
                    catch (std::exception &e)
                    {
                        logger->error("Error loading dataset! Maybe not a valid dataset? Details : %s", e.what());
                    }
                }
            };
            file_open_thread = std::thread(fun);
        }
    }
};
