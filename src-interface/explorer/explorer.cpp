#include "explorer.h"
// #include "image_handler.h"
// #include "radiation_handler.h"
// #include "scatterometer_handler.h"
// #include "core/config.h"
// #include "products/dataset.h"
#include "common/utils.h"
// #include "core/resources.h"
#include "core/plugin.h"

#include "core/style.h"
#include "dsp/cyclo_test.h"
#include "dsp/flowgraph/dsp_flowgraph_handler.h"
#include "handlers/dataset/dataset_handler.h"
#include "handlers/dummy_handler.h"
#include "handlers/product/image_product_handler.h" // TODOREWORK CLEAN
#include "handlers/projection/projection_handler.h"
#include "handlers/trash/trash_handler.h"
#include "handlers/vector/shapefile_handler.h"
// TODOREWORK
#include "core/resources.h"
#include "imgui/imgui.h"

// TODOREWORK
#include "dsp/newrec.h"
#include "dsp/waterfall_test.h"

#include "image/io.h"

#include "imgui/imgui_filedrop.h"
#include "main_ui.h"

namespace satdump
{
    namespace explorer
    {
        ExplorerApplication::ExplorerApplication() : Application("explorer")
        {
            master_handler = std::make_shared<handlers::DummyHandler>("MasterHandlerExplorer");

            // Add trashcan
            trash_handler = std::make_shared<handlers::DummyHandler>("TrashHandlerExplorer");
            auto trash_h = std::make_shared<handlers::TrashHandler>();
            trash_h->setCanBeDragged(false);
            trash_handler->addSubHandler(trash_h);
            trash_handler->setCanBeDraggedTo(false);

            // Add processing handler "spot"
            processing_handler = std::make_shared<handlers::DummyHandler>("ProcessingHandlerExplorer");
            processing_handler_sub = std::make_shared<handlers::DummyHandler>("Processing");
            processing_handler_sub->setCanBeDragged(false);
            processing_handler->addSubHandler(processing_handler_sub);
            processing_handler->setCanBeDraggedTo(false);

            // Enable dropping files onto the explorer. TODOREWORK, check the explorer IS active!?
            eventBus->register_handler<imgui_utils::FileDropEvent>(
                [this](const imgui_utils::FileDropEvent &v)
                {
                    for (auto &f : v.files)
                        tryOpenFileInExplorer(f);
                });

            // Enable adding handlers to the explorer externally
            eventBus->register_handler<ExplorerAddHandlerEvent>(
                [this](const ExplorerAddHandlerEvent &e)
                {
                    if (e.is_processing)
                        processing_handler_sub->addSubHandler(e.h);
                    else
                        master_handler->addSubHandler(e.h);
                    if (e.open)
                        curr_handler = e.h;
                });

            // TODOREWORK. Returns the last selected handler of a specific type if available
            eventBus->register_handler<GetLastSelectedOfTypeEvent>(
                [this](const GetLastSelectedOfTypeEvent &v)
                {
                    if (last_selected_handler.count(v.type))
                        v.h = last_selected_handler[v.type];
                    else
                        v.h = nullptr;
                });

            // TODOREWORK remove
            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/metop_test/dataset.json");
        }

        ExplorerApplication::~ExplorerApplication()
        {
            if (file_open_thread.joinable())
                file_open_thread.join();
        }

        void ExplorerApplication::drawPanel()
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto prev_curr = curr_handler;

                if (ImGui::BeginTable("##masterhandlertable", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("##masterhandlertable_col", ImGuiTableColumnFlags_None);
                    ImGui::TableNextColumn();

                    if (processing_handler_sub->hasSubhandlers())
                    {
                        processing_handler->drawTreeMenu(curr_handler);
                        for (auto &h : processing_handler_sub->getAllSubHandlers())
                            if (h->getName() == "PROCESSING_DONE") // TODOREWORK MASSIVE HACK
                                processing_handler_sub->delSubHandler(h);
                    }
                    master_handler->drawTreeMenu(curr_handler);
                    trash_handler->drawTreeMenu(curr_handler);

                    ImGui::EndTable();
                }

                if (prev_curr != curr_handler)
                    last_selected_handler.insert_or_assign(curr_handler->getID(), curr_handler);
            }

            if (curr_handler)
                curr_handler->drawMenu();
        }

        void ExplorerApplication::drawMenuBar()
        {
            // Handle File Stuff TODOREWORK?
            {
                if (file_open_dialog.update())
                {
                    std::string prod_path = file_open_dialog.getPath();
                    if (prod_path.size() > 0)
                        tryOpenFileInExplorer(prod_path);
                    else
                        logger->trace("No file selected");
                }
            }

            if (ImGui::BeginMenuBar())
            {
                // Main "Open" menu, for files, other handlers, etc
                if (ImGui::BeginMenu("File"))
                {
                    file_open_dialog.render("Open File", "Open File", "", {{"All Files", "*"}});

                    if (ImGui::BeginMenu("Quick Open"))
                    {
                        ImGui::InputText("##quickvieweropen", &quickOpenString);
                        bool go = ImGui::IsItemDeactivatedAfterEdit();
                        ImGui::SameLine();
                        if (ImGui::Button("Go##loadfromhttpgo") || go)
                        {
                            tryOpenFileInExplorer(quickOpenString);
                            quickOpenString.clear();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Paste & Load"))
                            tryOpenFileInExplorer(std::string(ImGui::GetClipboardText()));
                        ImGui::EndMenu();
                    }

                    // TODOREWORK remove
                    if (ImGui::BeginMenu("Hardcoded"))
                    {
                        if (ImGui::MenuItem("Load KMSS"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/dataset.json");
                        if (ImGui::MenuItem("Load Sterna"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/aws_pfm_cadu/dataset.json");
                        if (ImGui::MenuItem("Load MSUGS"))
                            tryOpenFileInExplorer("/home/alan/Downloads/20241231_132953_ARKTIKA-M 2_dat/MSUGS_VIS1/product.cbor");
                        if (ImGui::MenuItem("Load MSUGS 2"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/20250104_071415_ELEKTRO-L_3_dat/dataset.json");
                        if (ImGui::MenuItem("Load MetOp"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/metop_test/dataset.json");
                        if (ImGui::MenuItem("Load L3"))
                            tryOpenFileInExplorer("/data_ssd/ELEKTRO-L3/20250104_071415_ELEKTRO-L 3.dat_OUT/dataset.json");
                        if (ImGui::MenuItem("Load JPSS-1"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/202411271228_NOAA_20/dataset.json");
                        if (ImGui::MenuItem("Load JPSS-2"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/n21_day/dataset.json");
                        if (ImGui::MenuItem("Load APT"))
                            tryOpenFileInExplorer("/home/alan/Downloads/audio_137912500Hz_11-39-56_04-03-2024_wav/dataset.json");
                        if (ImGui::MenuItem("Load GOES"))
                            tryOpenFileInExplorer("/home/alan/Downloads/SatDump_NEWPRODS/goes_hrit_jvital2013_cadu/IMAGES/GOES-16/Full Disk/2024-04-17_18-00-20/product.cbor");
                        if (ImGui::MenuItem("Load Shapefile"))
                        {
                            auto shp_h = std::make_shared<handlers::ShapefileHandler>(resources::getResourcePath("maps/ne_10m_admin_0_countries.shp"));
                            master_handler->addSubHandler(shp_h);
                        }

                        if (ImGui::MenuItem("Load Shapefile FRA_1"))
                        {
                            auto shp_h = std::make_shared<handlers::ShapefileHandler>("/home/alan/Downloads/gadm41_FRA_shp/gadm41_FRA_1.shp");
                            master_handler->addSubHandler(shp_h);
                        }

                        if (ImGui::MenuItem("Load Shapefile FRA_2"))
                        {
                            auto shp_h = std::make_shared<handlers::ShapefileHandler>("/home/alan/Downloads/gadm41_FRA_shp/gadm41_FRA_2.shp");
                            master_handler->addSubHandler(shp_h);
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Tools"))
                    { // TODOREWORK?
                        if (ImGui::MenuItem("Projection"))
                            master_handler->addSubHandler(std::make_shared<handlers::ProjectionHandler>());
                        if (ImGui::MenuItem("DSP Flowgraph"))
                            master_handler->addSubHandler(std::make_shared<handlers::DSPFlowGraphHandler>());
                        if (ImGui::MenuItem("Waterfall TEST"))
                            master_handler->addSubHandler(std::make_shared<handlers::WaterfallTestHandler>());
                        if (ImGui::MenuItem("NewRec TEST"))
                            master_handler->addSubHandler(std::make_shared<handlers::NewRecHandler>());
                        if (ImGui::MenuItem("CycloHelper TEST"))
                            master_handler->addSubHandler(std::make_shared<handlers::CycloHelperHandler>());
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Handler", bool(curr_handler)))
                {
                    if (curr_handler && ImGui::BeginMenu("Config"))
                    {
                        if (ImGui::MenuItem("JSON To Clipboard"))
                        {
                            ImGui::SetClipboardText(curr_handler->getConfig().dump(4).c_str());
                            logger->warn("Copied to clipboard!");
                        }
                        if (ImGui::MenuItem("JSON From Clipboard"))
                        {
                            curr_handler->setConfig(nlohmann::json::parse(ImGui::GetClipboardText()));
                            logger->warn("Copied from clipboard!");
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                // TODOREWORK
                if (curr_handler)
                {
                    if (ImGui::BeginMenu("Current"))
                    {
                        curr_handler->drawMenuBar();
                        ImGui::EndMenu();
                    }
                }

                eventBus->fire_event<RenderLoadMenuElementsEvent>({curr_handler, master_handler});

                ImGui::EndMenuBar();
            }
        }

        void ExplorerApplication::drawContents() { ImGui::Text("No handler selected!"); }

        void ExplorerApplication::drawUI()
        {
            drawMenuBar();

            ImVec2 explorer_size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginTable("##wiever_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, explorer_size.x * panel_ratio);
                ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, explorer_size.x * (1.0f - panel_ratio));
                ImGui::TableNextColumn();

                float left_width = ImGui::GetColumnWidth(0);
                float right_width = explorer_size.x - left_width;
                if (left_width != last_width && last_width != -1)
                    panel_ratio = left_width / explorer_size.x;
                last_width = left_width;

                ImGui::BeginChild("ExplorerChildPanel", {left_width, float(explorer_size.y - 10)}, false);
                drawPanel();
                ImGui::EndChild();

                ImGui::TableNextColumn();
                ImGui::BeginGroup();

                if (curr_handler)
                    curr_handler->drawContents({float(right_width - 9 * ui_scale), float(explorer_size.y)});
                else
                    drawContents();

                ImGui::EndGroup();
                ImGui::EndTable();
            }
        }

        void ExplorerApplication::tryOpenFileInExplorer(std::string path)
        {
            if (file_open_thread.joinable())
                file_open_thread.join();
            auto fun = [this, path]()
            {
                eventBus->fire_event<SetIsProcessingEvent>({});

                try
                {
                    if (!std::filesystem::path(path).has_extension())
                    {
                        logger->error("Invalid file, no extension!");
                        return;
                    }

                    if (std::filesystem::path(path).extension().string() == ".cbor")
                    {
                        logger->trace("Explorer loading product " + path);

                        try
                        {
                            auto prod_h = handlers::getProductHandlerForProduct(products::loadProduct(path));
                            master_handler->addSubHandler(prod_h);
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Error loading product! Maybe not a valid product? (Reminder : Pre-2.0.0 products are NOT compatible with 2.0.0) Details : %s", e.what());
                        }
                    }
                    else if (std::filesystem::path(path).extension().string() == ".json")
                    {
                        logger->trace("Viewer loading dataset " + path);

                        try
                        {
                            products::DataSet dataset;
                            dataset.load(path);
                            std::shared_ptr<handlers::DatasetHandler> dat_h = std::make_shared<handlers::DatasetHandler>(std::filesystem::path(path).parent_path().string(), dataset);
                            master_handler->addSubHandler(dat_h);
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Error loading dataset! Maybe not a valid dataset? (Reminder : Pre-2.0.0 datasets are NOT compatible with 2.0.0) Details : %s", e.what());
                        }
                    }
                    else if (std::filesystem::path(path).extension().string() == ".shp")
                    {
                        logger->trace("Viewer loading shapefile " + path);

                        master_handler->addSubHandler(std::make_shared<handlers::ShapefileHandler>(path));
                    }
                    else
                    {
                        logger->trace("Viewer loading image " + path);

                        image::Image img;
                        image::load_img(img, path);
                        if (img.size() > 0)
                            master_handler->addSubHandler(std::make_shared<handlers::ImageHandler>(img, std::filesystem::path(path).stem().string()));
                        else
                            logger->error("Could not open this file as image!");
                    }
                }
                catch (std::exception &e)
                {
                    logger->error("Error opening file! => %s", e.what());
                }

                eventBus->fire_event<SetIsDoneProcessingEvent>({});
            };
            file_open_thread = std::thread(fun);
        }
    } // namespace explorer
}; // namespace satdump
