#include "explorer.h"
// #include "image_handler.h"
// #include "radiation_handler.h"
// #include "scatterometer_handler.h"
// #include "core/config.h"
// #include "products/dataset.h"
#include "common/utils.h"
// #include "core/resources.h"
#include "core/exception.h"
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
        ExplorerApplication::ExplorerApplication()
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
            processing_handler->setCanBeDraggedTo(false);

            // Enable dropping files onto the explorer.
            eventBus->register_handler<imgui_utils::FileDropEvent>(
                [this](const imgui_utils::FileDropEvent &v)
                {
                    for (auto &f : v.files)
                        tryOpenFileInExplorer(f);
                });

            // Enable adding handlers to the explorer externally
            eventBus->register_handler<ExplorerAddHandlerEvent>([this](const ExplorerAddHandlerEvent &e) { addHandler(e.h, e.open, e.is_processing); });

            // Returns the last selected handler of a specific type if available
            eventBus->register_handler<GetLastSelectedOfTypeEvent>(
                [this](const GetLastSelectedOfTypeEvent &v)
                {
                    if (last_selected_handler.count(v.type))
                        v.h = last_selected_handler[v.type];
                    else
                        v.h = nullptr;
                });
        }

        ExplorerApplication::~ExplorerApplication() {}

        void ExplorerApplication::addHandler(std::shared_ptr<handlers::Handler> h, bool open, bool is_processing)
        {
            if (is_processing)
                processing_handler->addSubHandler(h);
            else
            {
                // Try adding to a group if possible
                bool added = false;
                for (auto &v : group_definitions)
                {
                    for (auto &v2 : v.second)
                    {
                        if (v2 == h->getID())
                        {
                            if (groups_handlers.count(v.first) == 0)
                                groups_handlers.emplace(v.first, std::make_shared<handlers::DummyHandler>(v.first + "HandlerExplorer"));
                            groups_handlers[v.first]->addSubHandler(h);
                            added = true;
                        }
                    }
                }

                if (!added)
                    master_handler->addSubHandler(h);
            }
            if (open)
                curr_handler = h;
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

                    bool rendering_separators = false;

                    if (processing_handler->hasSubhandlers())
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SeparatorText("Processing");

                        processing_handler->drawTreeMenu(curr_handler);
                        for (auto &h : processing_handler->getAllSubHandlers())
                            if (h->getName() == "PROCESSING_DONE") // TODOREWORK MASSIVE HACK
                                processing_handler->delSubHandler(h);

                        rendering_separators = true;
                    }

                    for (auto &v : groups_handlers)
                    {
                        if (v.second->hasSubhandlers())
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::SeparatorText(v.first.c_str());

                            v.second->drawTreeMenu(curr_handler);
                            rendering_separators = true;
                        }
                    }

                    if (rendering_separators)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SeparatorText("Others");
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

                    if (ImGui::BeginMenu("Tools"))
                    { // TODOREWORK?
                        if (ImGui::MenuItem("Projection"))
                            addHandler(std::make_shared<handlers::ProjectionHandler>());
                        if (ImGui::MenuItem("DSP Flowgraph"))
                            addHandler(std::make_shared<handlers::DSPFlowGraphHandler>());
                        if (ImGui::MenuItem("Waterfall TEST"))
                            addHandler(std::make_shared<handlers::WaterfallTestHandler>());
                        if (ImGui::MenuItem("NewRec TEST"))
                            addHandler(std::make_shared<handlers::NewRecHandler>());
                        if (ImGui::MenuItem("CycloHelper TEST"))
                            addHandler(std::make_shared<handlers::CycloHelperHandler>());
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

        void ExplorerApplication::draw()
        {
            drawMenuBar();

            ImVec2 explorer_size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginTable("##explorer_table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
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
            auto fun = [this, path]()
            {
                eventBus->fire_event<SetIsProcessingEvent>({});

                try
                {
                    // Get available loaders
                    std::vector<std::pair<std::string, std::function<void(std::string, ExplorerApplication *)>>> loaders;

                    // Default stuff
                    if (std::filesystem::path(path).extension().string() == ".cbor")
                        loaders.push_back({"Product Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Explorer loading product " + path);

                                               try
                                               {
                                                   auto prod_h = handlers::getProductHandlerForProduct(products::loadProduct(path));
                                                   e->addHandler(prod_h);
                                               }
                                               catch (std::exception &e)
                                               {
                                                   logger->error("Error loading product! Maybe not a valid product? (Reminder : Pre-2.0.0 products are NOT compatible with 2.0.0) Details : %s",
                                                                 e.what());
                                               }
                                           }});
                    else if (std::filesystem::path(path).extension().string() == ".json")
                        loaders.push_back(
                            {"Dataset Loader", [](std::string path, ExplorerApplication *e)
                             {
                                 logger->trace("Viewer loading dataset " + path);

                                 try
                                 {
                                     products::DataSet dataset;
                                     dataset.load(path);
                                     std::shared_ptr<handlers::DatasetHandler> dat_h = std::make_shared<handlers::DatasetHandler>(std::filesystem::path(path).parent_path().string(), dataset);
                                     e->addHandler(dat_h);
                                 }
                                 catch (std::exception &e)
                                 {
                                     logger->error("Error loading dataset! Maybe not a valid dataset? (Reminder : Pre-2.0.0 datasets are NOT compatible with 2.0.0) Details : %s", e.what());
                                 }
                             }});
                    else if (std::filesystem::path(path).extension().string() == ".shp")
                        loaders.push_back({"Shapefile Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Viewer loading shapefile " + path);
                                               e->addHandler(std::make_shared<handlers::ShapefileHandler>(path));
                                           }});
#if 0 // TODOREWORK ADD MENU
                                           else if (std::filesystem::path(path).has_extension())
                        loaders.push_back({"Image Loader", [](std::string path, ExplorerApplication *e)
                                           {
                                               logger->trace("Viewer loading image " + path);

                                               image::Image img;
                                               image::load_img(img, path);
                                               if (img.size() > 0)
                                                   e->addHandler(std::make_shared<handlers::ImageHandler>(img, std::filesystem::path(path).stem().string()));
                                               else
                                                   logger->error("Could not open this file as image!");
                                           }});
#endif

                    // Plugin loaders
                    eventBus->fire_event<ExplorerRequestFileLoad>({std::filesystem::path(path).stem().string() + std::filesystem::path(path).extension().string(), loaders});

                    // Load it
                    if (loaders.size() == 1)
                    {
                        loaders[0].second(path, this);
                    }
                    else if (loaders.size() > 1)
                    {
                        throw satdump_exception("More than one loader available! TODO SELECTION MENU!");
                    }
                    else
                    {
                        throw satdump_exception("No loader found for file : " + path + "!");
                    }
                }
                catch (std::exception &e)
                {
                    logger->error("Error opening file! => %s", e.what());
                }

                eventBus->fire_event<SetIsDoneProcessingEvent>({});
            };
            file_open_queue.push(fun);
        }
    } // namespace explorer
}; // namespace satdump
